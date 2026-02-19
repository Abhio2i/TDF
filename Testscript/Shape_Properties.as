void main(ScriptEngine@ e)
{
    e.useCity("Bhopal");

    e.canvasAddRectangle("Block", 10, 6);

    e.sleep(500);

    // green border, thickness = 4
    e.addShapeProperties("TempRectangle_0", 0, 255, 0, 4);

    // Generate PDF report
    e.generatePDFReport("GIS_Report_Properties.pdf");
}
