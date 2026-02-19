void main(ScriptEngine@ e)
{
    // Bitmap location
    double lon = 78.02232;
    double lat = 27.14927;

    // Place bitmap on canvas
    e.onBitmapSelected("Hospital", lon, lat);

    // Generate PDF report
    e.generatePDFReport("bitmap_Report.pdf");
}
