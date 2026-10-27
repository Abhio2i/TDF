void main(ScriptEngine@ e) {

     // add layer
    e.AddLayer("Layer1");

    // Plot a point on the canvas
    e.AddPoint("Point", 28.59, 78.34);

    Print(" Landmark Point marked.");

    // Wait for 2 seconds
    e.sleep(2000);

    // Generate PDF report
    e.generatePDFReport("Point_GIS_Report.pdf");
}
