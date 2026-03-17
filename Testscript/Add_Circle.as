void main(ScriptEngine@ e)
{
    // add layer
    e.canvasCreateVectorLayer("Layer1");

    // Set city context
    e.useCity("Bhopal");

    // Add circle on canvas
    e.canvasAddCircle("Circle", 10000.0);

    // Generate PDF report
    e.generatePDFReport("GIS_Report_circle.pdf");
}
