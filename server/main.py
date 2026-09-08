import uvicorn

from fastapi import FastAPI

from config import AppConfig

from services.pdf_converter import (
    PDFConverterService,
)

from services.file_service import (
    FileService,
)

from routes.frontend import (
    create_frontend_router,
)

from routes.pdf import (
    create_pdf_router,
)


def create_app() -> FastAPI:
    """Create and configure the FastAPI application."""

    config = AppConfig()

    converter_service = PDFConverterService(
        config
    )

    file_service = FileService(
        config
    )

    app = FastAPI(
        title="PDF Converter"
    )

    app.include_router(
        create_frontend_router(config)
    )

    app.include_router(
        create_pdf_router(
            converter_service,
            file_service,
        )
    )

    return app


app = create_app()


if __name__ == "__main__":

    uvicorn.run(
        app,
        host="0.0.0.0",
        port=8000,
    )
