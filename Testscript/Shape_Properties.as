void main(ScriptEngine@ e)
{
    e.useCity("Bhopal");

    // Create rectangle
    e.AddRectangle("Block", 10000, 6000);

    e.sleep(500);

    // Green border, thickness = 4
    e.addShapeProperties(
        "Block",
        0,
        255,
        0,
        4
    );

    // Generate PDF report
    e.generatePDFReport(
        "GIS_Report_Properties.pdf"
    );
}