from pathlib import Path
import subprocess

from fastapi import UploadFile

from config import AppConfig


class PDFConverterService:
    """Handles PDF conversion using the C++ executable."""

    def __init__(self, config: AppConfig):
        self.config = config

    def validate_executable(self) -> None:
        """Ensure the C++ converter exists."""

        if not self.config.cpp_executable.is_file():
            raise FileNotFoundError(
                "C++ executable not found at: "
                f"{self.config.cpp_executable}"
            )

    async def save_temp_file(
        self,
        file: UploadFile,
    ) -> Path:
        """Save uploaded PDF to a temporary file."""

        filename = Path(file.filename).name

        temp_path = (
            self.config.base_dir / f"temp_{filename}"
        )

        content = await file.read()

        with open(temp_path, "wb") as output_file:
            output_file.write(content)

        return temp_path

    def create_output_directory(
        self,
        filename: str,
    ) -> Path:
        """Create the output directory for a PDF."""

        filename_no_ext = Path(filename).stem

        output_dir = (
            self.config.output_base_dir
            / f"out_{filename_no_ext}"
        )

        output_dir.mkdir(
            parents=True,
            exist_ok=True,
        )

        return output_dir

    def process_pdf(
        self,
        input_pdf: Path,
        output_dir: Path,
    ) -> None:
        """Execute the C++ converter."""

        self.validate_executable()

        process = subprocess.run(
            [
                str(self.config.cpp_executable),
                str(output_dir),
                str(input_pdf),
            ],
            capture_output=True,
            text=True,
        )

        if process.returncode != 0:
            raise RuntimeError(
                "C++ conversion failed:\n"
                f"{process.stderr}"
            )

    def get_binfiles_directory(
        self,
        output_dir: Path,
    ) -> Path:
        """Return the binfiles directory."""

        return output_dir / "binfiles"

    def get_binfiles(
        self,
        output_dir: Path,
    ) -> list[Path]:
        """Return all generated BIN files."""

        binfiles_dir = self.get_binfiles_directory(
            output_dir
        )

        if not binfiles_dir.is_dir():
            return []

        return [
            path
            for path in binfiles_dir.iterdir()
            if path.is_file()
        ]

    async def convert(
        self,
        file: UploadFile,
    ) -> dict:
        """Convert an uploaded PDF."""

        if not file.filename.lower().endswith(".pdf"):
            raise ValueError(
                "Only PDF files are allowed."
            )

        output_dir = self.create_output_directory(
            file.filename
        )

        temp_file = await self.save_temp_file(file)

        try:
            self.process_pdf(
                temp_file,
                output_dir,
            )

            binfiles_dir = (
                self.get_binfiles_directory(
                    output_dir
                )
            )

            if not binfiles_dir.is_dir():
                raise RuntimeError(
                    "The C++ converter did not create "
                    f"the binfiles directory: {binfiles_dir}"
                )

            binfiles = self.get_binfiles(
                output_dir
            )

            return {
                "message": (
                    f"Successfully processed "
                    f"'{file.filename}'!"
                ),
                "output_directory": str(output_dir),
                "binfiles_directory": str(
                    binfiles_dir
                ),
                "binfiles": [
                    file.name
                    for file in binfiles
                ],
                "download_url": (
                    "/download/"
                    f"{output_dir.name}"
                ),
            }

        finally:
            if temp_file.exists():
                temp_file.unlink()
