void main(ScriptEngine@ e)
{
    // ================= COORDINATE SYSTEM =================

    // 1️⃣ Latitude / Longitude (WGS84 – EPSG:4326)
   //e.switchCoordinateSystem("latlon");

    // 2️⃣ Auto UTM (zone auto-detected from cursor/location)
    //e.switchCoordinateSystem("utm");

    // 3️⃣ MGRS (Military Grid Reference System)
    e.switchCoordinateSystem("mgrs");

    // ================= REPORT =================

    // Screenshot 
    e.generatePDFReport("Map_Coordinate_System_Report.pdf");
}
