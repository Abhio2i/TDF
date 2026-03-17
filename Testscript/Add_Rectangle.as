void main(ScriptEngine@ e)
{
    // add layer
    e.canvasCreateVectorLayer("Layer1");

    // Set city context
    e.useCity("Bhopal");

    // Add rectangle on canvas
    e.canvasAddRectangle("Grid_Block", 80000.0, 60000.0);

    // Generate PDF report
    e.generatePDFReport("GIS_Report_rectange.pdf");
}
