void main(ScriptEngine@ e) {

    // Plot a point on the canvas
    e.canvasAddPoint("Point", 78.34, 28.59);

    Print(" > Landmark Point marked.");

    // Wait for 2 seconds
    e.sleep(2000);

    // Generate PDF report
    e.generatePDFReport("Point_GIS_Report.pdf");
}
