void main(ScriptEngine@ e)
{
    // Bitmap location
    double lat = 27.14927;
    double lon = 78.02232;

    // Place bitmap on canvas
    e.onBitmapSelected("Hospital", lat, lon);

    // Generate PDF report
    e.generatePDFReport("bitmap_Report.pdf");
}
