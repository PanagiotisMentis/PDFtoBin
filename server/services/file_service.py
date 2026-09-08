from pathlib import Path
import shutil
import tempfile

from config import AppConfig


class FileService:
    """Handles generated files and downloads."""

    def __init__(self, config: AppConfig):
        self.config = config

    def get_binfiles_directory(
        self,
        output_directory_name: str,
    ) -> Path:
        """
        Get the binfiles directory for an output directory.
        """

        # Prevent path traversal.
        safe_name = Path(
            output_directory_name
        ).name

        if safe_name != output_directory_name:
            raise ValueError(
                "Invalid output directory name."
            )

        output_dir = (
            self.config.output_base_dir
            / safe_name
        )

        binfiles_dir = output_dir / "binfiles"

        return binfiles_dir

    def create_zip(
        self,
        output_directory_name: str,
    ) -> Path:
        """Create a ZIP containing all BIN files."""

        binfiles_dir = (
            self.get_binfiles_directory(
                output_directory_name
            )
        )

        if not binfiles_dir.is_dir():
            raise FileNotFoundError(
                "binfiles directory not found."
            )

        # Make sure there are actually files.
        binfiles = [
            path
            for path in binfiles_dir.iterdir()
            if path.is_file()
        ]

        if not binfiles:
            raise FileNotFoundError(
                "No BIN files found."
            )

        safe_name = Path(
            output_directory_name
        ).name

        zip_base = (
            Path(tempfile.gettempdir())
            / f"{safe_name}_binfiles"
        )

        zip_path = shutil.make_archive(
            str(zip_base),
            "zip",
            str(binfiles_dir),
        )

        return Path(zip_path)
