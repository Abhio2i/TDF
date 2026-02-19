void main(ScriptEngine@ e) {
    Print("\n## Section: Plotting Polygon over India ##");

    // Format: "Longitude, Latitude" 
    array<string> indiaPoints = {
        "77.0, 30.0", // North
        "78.11, 29.36", // East
        "77.23, 29.10", // South
        "76.64, 29.51", // West
        "77.0, 30.0"  // Closing point (to make it look connected)
    };

    // draw polygen and connect all points
    e.canvasAddPolygon("India_Strategic_Zone", indiaPoints);

   // Generate PDF report
    e.generatePDFReport("GIS_Report_Polygon.pdf");
}