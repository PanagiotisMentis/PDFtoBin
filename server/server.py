import os
import subprocess
from typing import Dict, Any
import uvicorn
from fastapi import FastAPI, UploadFile, File, HTTPException
from fastapi.responses import HTMLResponse
# import paho.mqtt.client as mqtt


class PDFConverterService:
    """Service class responsible for executing PDF to binary conversion."""

    def __init__(
        self,
        cpp_executable: str = "../converter_app/build/PDFtoBin.exe",
        output_base_dir: str = "./output_directories",
    ):
        self.cpp_executable = os.path.abspath(cpp_executable)
        self.output_base_dir = os.path.abspath(output_base_dir)
        os.makedirs(self.output_base_dir, exist_ok=True)

    def _validate_executable(self) -> None:
        """Ensure the C++ executable exists prior to execution."""
        if not os.path.isfile(self.cpp_executable):
            raise FileNotFoundError(
                f"C++ executable not found at: {self.cpp_executable}"
            )

    async def _save_temp_file(self, file: UploadFile) -> str:
        """Save uploaded file temporarily to disk."""
        temp_path = f"temp_{file.filename}"
        with open(temp_path, "wb") as f:
            content = await file.read()
            f.write(content)
        return temp_path

    def process_pdf(self, input_pdf_path: str, target_output_dir: str) -> None:
        """Execute the C++ converter binary via subprocess."""
        self._validate_executable()

        process = subprocess.run(
            [self.cpp_executable, target_output_dir, input_pdf_path],
            capture_output=True,
            text=True,
        )

        if process.returncode != 0:
            raise RuntimeError(f"C++ Conversion failed: {process.stderr}")

    async def convert(self, file: UploadFile) -> Dict[str, Any]:
        """Orchestrate saving, processing, and output management."""
        if not file.filename.endswith(".pdf"):
            raise ValueError("Only PDF files are allowed.")

        filename_no_ext = os.path.splitext(file.filename)[0]
        target_output_dir = os.path.join(
            self.output_base_dir, f"out_{filename_no_ext}"
        )
        temp_pdf_path = await self._save_temp_file(file)

        try:
            # Run C++ processing
            self.process_pdf(temp_pdf_path, target_output_dir)

            # -------------------------------------------------------------
            # MQTT Sending (Place MQTT publishing method here when ready)
            # -------------------------------------------------------------

            return {
                "message": f"Successfully processed '{file.filename}'!",
                "output_directory": target_output_dir,
            }

        finally:
            # Clean up temporary PDF file
            if os.path.exists(temp_pdf_path):
                os.remove(temp_pdf_path)


class AppManager:
    """Class to configure and construct the FastAPI application instance."""

    def __init__(self):
        self.app = FastAPI()
        self.converter_service = PDFConverterService()
        self._connect_frontend()

    def _connect_frontend(self) -> None:
        @self.app.get("/", response_class=HTMLResponse)
        async def get_index():
            index_path = os.path.join("frontend", "index.html")
            if not os.path.exists(index_path):
                raise HTTPException(
                    status_code=404, detail="Frontend template not found."
                )
            with open(index_path, "r") as f:
                return f.read()

        @self.app.post("/upload_pdf")
        async def upload_pdf(file: UploadFile = File(...)):
            try:
                return await self.converter_service.convert(file)
            except ValueError as ve:
                raise HTTPException(status_code=400, detail=str(ve))
            except (FileNotFoundError, RuntimeError) as re:
                raise HTTPException(status_code=500, detail=str(re))
            except Exception as e:
                raise HTTPException(status_code=500, detail=f"Unexpected error: {str(e)}")


# Instantiation
app_builder = AppManager()
app = app_builder.app

# -------------------------------------------------------------
# Auto-run Uvicorn server when running 'python main.py'
# -------------------------------------------------------------
if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)