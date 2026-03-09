void main(ScriptEngine@ e)
{
    // Set city context
    e.useCity("Bhopal");

    // Add circle on canvas
    e.canvasAddCircle("Circle", 10000.0);

    // Generate PDF report
    e.generatePDFReport("GIS_Report_circle.pdf");
}
