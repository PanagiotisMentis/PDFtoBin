from fastapi import APIRouter, HTTPException
from fastapi.responses import HTMLResponse

from config import AppConfig


def create_frontend_router(
    config: AppConfig,
) -> APIRouter:

    router = APIRouter()

    @router.get(
        "/",
        response_class=HTMLResponse,
    )
    async def get_index():

        if not config.index_file.is_file():
            raise HTTPException(
                status_code=404,
                detail="Frontend template not found.",
            )

        return config.index_file.read_text(
            encoding="utf-8"
        )

    return router
