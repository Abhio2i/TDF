Choosing a Coordinate System
==============================

Overview
--------

You can choose the coordinate system used for searching, plotting, and interacting with your tactical map. The system supports switching between Lat/Lon (geodetic) and UTM coordinates directly on the application screen, allowing flexible state-wise search and visualization.

Setting the Coordinate System
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You may select the default coordinate system at launch, and switch modes dynamically while working in the Scenario Editor.

- To set the initial coordinate system, use the Configuration Manager within the application. Go to **Tools > Configuration Manager**, then select the **General** category, and adjust the "Initial Coordinate System" field. Supported choices are:
   - **GEODETIC (Lat/Lon coordinates)**
   - **UTM (Universal Transverse Mercator)**
- Save your preferences, restart the Scenario Manager, and the system will initialize to your selected CRS.

Switching Coordinate Systems
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On the tactical display screen, the coordinate system indicator shows the current mode (Lat/Lon or UTM). You can switch between modes as needed:

- **Lat/Lon Mode:**  
  Use geographic coordinates (latitude and longitude in EPSG:4326/WGS84) to search and display locations state-wise. Enter coordinates or place names to center and zoom on specific areas.
- **UTM Mode:**  
  Enables a metric grid overlay and displays coordinates in meters according to the relevant UTM zone, automatically detected based on map center. Location searches and visualizations now use meter-based offsets instead of degrees.

State-wise Search
~~~~~~~~~~~~~~~~~

You can search for locations by state using latitude/longitude coordinates. After entering a state or coordinate query, the system:
  - Fetches matching locations.
  - Auto-centers and zooms the map to the relevant region.
  - Highlights boundaries and overlays the appropriate grid (UTM or geodetic).

Switching to UTM will immediately update the coordinate display on the tactical map and adjust your search and measurement context to the metric grid.

