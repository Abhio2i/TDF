void main(ScriptEngine@ e) {
    Print("\n## Section: Plotting Polygon over India ##");

     // add layer
    e.AddLayer("Layer1");

    // Format: "Longitude, Latitude" 
    array<string> indiaPoints = {
        "30.0, 77.0", // North
        "29.36, 78.11", // East
        "29.10, 77.23", // South
        "29.51, 76.64", // West
        "30.0, 77.0"  // Closing point (to make it look connected)
    };

    // draw polygen and connect all points
    e.AddPolygon("India_Strategic_Zone", indiaPoints);

   // Generate PDF report
    e.generatePDFReport("GIS_Report_Polygon.pdf");
}