Installation Steps 
=====================

The following steps describe how to install and run the TDF application on 
Ubuntu 24.04.3.

1. Installing Required Dependency
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Before running the TDF AppImage, install the required *libfuse2* package.

Navigate to the directory where the ``libfuse2t64_2.9.9-9_amd64.deb`` file is located and run:

.. code-block:: bash

   sudo apt install ./libfuse2t64_2.9.9-9_amd64.deb

This installs the FUSE compatibility library required for AppImage execution.

2. Running the TDF AppImage
~~~~~~~~~~~~~~~~~~~~~~~~~~

After installing the dependency, navigate to the directory containing 
the ``TDF-x86_64.AppImage`` file.

Run the AppImage with a custom library path using:

.. code-block:: bash

   LD_LIBRARY_PATH=./TDF.AppDir/usr/lib ./TDF-x86_64.AppImage

This ensures that the AppImage loads the correct internal libraries during execution.

3. Successful Launch
~~~~~~~~~~~~~~~~~~~

If the above steps execute without errors, the TDF application should launch 
successfully. Ensure that the AppImage has executable permissions.

