Installation Instructions

    Download an appropriate LJM installer package from our download links above.

    Navigate to the file location in which you downloaded the package from a terminal session.

    Extract the package using the tar or unzip command:

        Extract a .tar.gz archive: tar -zvxf [installer package name here].tar.gz

        Extract a .zip archive: unzip [installer package name here].zip

    Navigate (cd) into the installer directory: cd [installer package name here]

    Run the install script with sudo: sudo ./labjack_ljm_installer.run

Optional Installer Arguments

The LJM Linux Installers are makeself archives and therefore support the makeself arguments. One useful set of arguments is --noexec --target <targetDir> which allows you to extract the archive contents to targetDir for analysis:

sudo ./labjack_ljm_installer.run --noexec --target <targetDir>
Optional Installer Setup Script Arguments

Arguments can be passed to the underlying LabJack setup script by adding a -- construct before any setup script arguments. The following setup script arguments are supported:

    --with-kipling: Install Kipling (Default).

    --without-kipling: Don't install Kipling.

    --restart-device-rules: Restart the udev device rules. (Default).

    --no-restart-device-rules: Don't restart the udev device rules. This is useful for Docker containers.

    --remove-extracted: Remove the extracted folder. (Default).

    --no-remove-extracted: Don't remove the extracted folder.

Example:

Install without Kipling.

sudo ./labjack_ljm_installer.run -- --without-kipling
