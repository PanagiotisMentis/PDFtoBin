from pathlib import Path


class AppConfig:
    """Application configuration and filesystem paths."""

    def __init__(self):
        # Directory containing this Python project.
        self.base_dir = Path(__file__).resolve().parent

        # C++ executable
        self.cpp_executable = (
            self.base_dir
            / "../converter_app/build/PDFtoBin.exe"
        ).resolve()

        # Generated PDF output directories
        self.output_base_dir = (
            self.base_dir / "output_directories"
        ).resolve()

        # Frontend
        self.frontend_dir = (
            self.base_dir / "frontend"
        ).resolve()

        self.index_file = (
            self.frontend_dir / "index.html"
        )

        # Make sure required directories exist.
        self.output_base_dir.mkdir(
            parents=True,
            exist_ok=True,
        )
