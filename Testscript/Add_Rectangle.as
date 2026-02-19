void main(ScriptEngine@ e)
{
    // Set city context
    e.useCity("Bhopal");

    // Add rectangle on canvas
    e.canvasAddRectangle("Grid_Block", 8.0, 6.0);

    // Generate PDF report
    e.generatePDFReport("GIS_Report_rectange.pdf");
}
