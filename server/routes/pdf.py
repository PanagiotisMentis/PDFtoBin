from fastapi import (
    APIRouter,
    File,
    HTTPException,
    UploadFile,
)
from fastapi.responses import FileResponse

from services.pdf_converter import (
    PDFConverterService,
)
from services.file_service import (
    FileService,
)


def create_pdf_router(
    converter_service: PDFConverterService,
    file_service: FileService,
) -> APIRouter:

    router = APIRouter()

    @router.post("/upload_pdf")
    async def upload_pdf(
        file: UploadFile = File(...),
    ):

        try:

            return await converter_service.convert(
                file
            )

        except ValueError as error:

            raise HTTPException(
                status_code=400,
                detail=str(error),
            )

        except FileNotFoundError as error:

            raise HTTPException(
                status_code=404,
                detail=str(error),
            )

        except RuntimeError as error:

            raise HTTPException(
                status_code=500,
                detail=str(error),
            )

        except Exception as error:

            raise HTTPException(
                status_code=500,
                detail=(
                    "Unexpected error: "
                    f"{error}"
                ),
            )

    @router.get(
        "/download/{output_directory}"
    )
    async def download_binfiles(
        output_directory: str,
    ):

        try:

            zip_path = file_service.create_zip(
                output_directory
            )

            return FileResponse(
                path=zip_path,
                filename=(
                    f"{output_directory}"
                    "_binfiles.zip"
                ),
                media_type="application/zip",
            )

        except ValueError as error:

            raise HTTPException(
                status_code=400,
                detail=str(error),
            )

        except FileNotFoundError as error:

            raise HTTPException(
                status_code=404,
                detail=str(error),
            )

        except Exception as error:

            raise HTTPException(
                status_code=500,
                detail=(
                    "Failed to create download: "
                    f"{error}"
                ),
            )

    return router
