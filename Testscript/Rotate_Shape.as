void main(ScriptEngine@ e)
{
    // Set city context
    e.useCity("Bhopal");

    // Add rectangle (width, height)
    e.canvasAddRectangle("Grid_Block", 40.0, 30.0);

    // Small delay so rectangle is created first
    e.sleep(3000);

    // Rotate rectangle by 45 degrees
    e.rotateShape("TempRectangle_0", 45.0);

    // Generate PDF report
    e.generatePDFReport("GIS_Report_rotate.pdf");
}
