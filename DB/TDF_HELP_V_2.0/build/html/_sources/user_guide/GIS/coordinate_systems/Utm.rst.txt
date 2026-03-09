Universal Transverse Mercator (UTM)
==============================

UTM coordinates
---------------

Universal Transverse Mercator (UTM) uses a grid built by defining zones on the Earth between 84 degrees North latitude and 80 degrees South latitude. Each zone is generally 6 degrees wide in longitude and 8 degrees wide in latitude. Each zone is designated by a zone number and a zone character.

UTM zone numbers define the longitudinal position of the zone. They go from 1 to 60 proceeding East from the 180th meridian from Greenwich. There are special UTM zones between 0 degrees and 36 degrees East longitude above 72 degrees latitude and a special zone 32 between 56 degrees and 64 degrees North latitude.

UTM zone characters are letters that designate the position of the zone along the North and South axes. Normally, these characters begin at 80 degrees South and proceed northward in 20 bands that are lettered C through X, omitting I and O. These bands are all 8 degrees wide except for band X, which is 12 degrees wide (between N72 and N84). However, in this GIS system, zone characters are not used. Instead, UTM coordinates have an "N:" or "S:" preceding the coordinate to indicate if the coordinate is located in the northern or southern hemisphere.

The geographic location of a point is then given by its x and y offsets in meters from the intersection point between the central meridian of the zone and the Equator, according to the UTM projection, where x is the offset in longitude and y is the offset in latitude. However, to always have positive offsets, the origin is initialized in the Northern Hemisphere to x = 500,000 meters, and y = 0 meters. In the Southern Hemisphere, the same point is the origin, but this time it is initialized to x = 500,000 meters and y = 10,000,000 meters.

UTM limitations
---------------

- When running in CDB mode, using the UTM coordinate system results in a loss of precision.
- The UTM grid system does not support exemption zones in this GIS implementation.
- Zone determination and transformations rely on QGIS CRS EPSG codes and error fallback to WGS84 when invalid.


