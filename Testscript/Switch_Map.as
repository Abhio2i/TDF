void main(ScriptEngine@ e)
{
    // 🔹 Satellite imagery view
    // High-resolution satellite tiles (best for visual confirmation)
    // e.canvasSwitchMap("satellite");

    // 🔹 Terrain / elevation view
    // Shows height, contours, relief (uses terrain/opentopo layer)
   e.canvasSwitchMap("tarrine");

    // 🔹 OpenStreetMap (default map)
    // Clean vector map: roads, cities, labels
   // e.canvasSwitchMap("osm");
    
    // generate pdf
    e.generatePDFReport("switchreport.pdf");
}
