void main(ScriptEngine@ e)
{
    ProfileCategaory@ p = e.NewProfile("AircraftProfile");
    
    // ==================================================
    // Jet1: Agra → New Delhi
    // ==================================================
    Platform@ jet1 = e.addEntityToPlatform(p, "Jet1");
    jet1.dynamicModel.moveSpeed = 1500;
    jet1.transform.setGeoCord(27.1767f, 78.0081f, 0.0f); // Agra
    jet1.trajectory.addWaypoint(27.1767f, 0.0f, 78.0081f); // Start
    jet1.trajectory.addWaypoint(28.6139f, 0.0f, 77.2090f); // New Delhi
    
    // Add IFF
    e.addIFFSubComponent(jet1, "IFF1");
    IFF@ jet1IFF = jet1.getIFFByName("IFF1");
    Print("✅ Added IFF1 to Jet1");
    
    // ==================================================
    // Jet2: Gurugram → Hathras
    // ==================================================
    Platform@ jet2 = e.addEntityToPlatform(p, "Jet2");
    jet2.dynamicModel.moveSpeed = 1500;
    jet2.transform.setGeoCord(28.4595f, 77.0266f, 0.0f); // Gurugram
    jet2.trajectory.addWaypoint(28.4595f, 0.0f, 77.0266f); // Start (Gurugram)
    jet2.trajectory.addWaypoint(27.5983f, 0.0f, 78.0506f); // End (Hathras)
    
    // Add IFF
    e.addIFFSubComponent(jet2, "IFF2");
    IFF@ jet2IFF = jet2.getIFFByName("IFF2");
    Print("✅ Added IFF2 to Jet2");
    
    // ==================================================
    // 🎯 AUTOMATED UI SETUP - Show Jet1 in IFF Display
    // ==================================================
    Print("🎯 Setting up automated display...");
    if (jet1IFF !is null) {
        e.selectEntityDisplay(jet1IFF);
        Print("✅ Sensors tab opened");
        Print("✅ IFF display selected");
        Print("✅ Jet1 selected in hierarchy");
        
        // Wait for display to render
        e.sleep(1000);
        
        // 📸 Capture initial screenshots (BOTH sensor + canvas)
        //e.captureSensorScreenshot("initial_iff.png");
        //Print("📸 Initial screenshots captured (IFF Display + Canvas)!");
    }
    
    // ==================================================
    // Simulation loop: check IFF targets every second
    // ==================================================
    SimStart(e);
    Print("🚀 Simulation started!");
    
    int jet1PrevCount = 0;
    int jet2PrevCount = 0;
    bool simulationStopped = false;
    string detectedByJet = "";
    
    for (int t = 0; t < 400; t++)
    {
        e.sleep(1000); // 1 second per tick
        
        // ---------------- Jet1 IFF ----------------
        if (jet1IFF !is null)
        {
            int targetCount = jet1IFF.getIFFTargetCount();
            if (targetCount > jet1PrevCount)
            {
                for (int i = jet1PrevCount; i < targetCount; i++)
                {
                    string responderId, responderName, mode, code;
                    float distance, angle;
                    int status;
                    
                    if (jet1IFF.getIFFTarget(i, responderId, responderName, mode, code, distance, angle, status))
                    {
                        string statusStr = (status == 1) ? "Friend" : "Foe";
                        
                        Print("[Jet1IFF] 🎯 New IFF contact detected #" + i 
                            + " | ID: " + responderId
                            + " | Name: " + responderName
                            + " | Status: " + statusStr
                            + " | Mode: " + mode
                            + " | Code: " + code
                            + " | Distance: " + distance + " km"
                            + " | Angle: " + angle + "°");
                        
                        // 🎯 Automatically switch to Jet1 display on detection
                        e.selectEntityDisplay(jet1IFF);
                        Print("📡 Switched to Jet1 IFF display");
                        
                        // Wait for display update
                        e.sleep(1000);
                        
                        // 📸 Capture DUAL screenshots (IFF Display + Canvas)
                        e.captureSensorScreenshot("jet1_iff_detection.png");
                        Print("📸 Jet1 IFF detection screenshots captured!");
                        Print("   → IFF display screenshot saved");
                        Print("   → Tactical canvas screenshot saved");
                        
                        detectedByJet = "Jet1";
                        simulationStopped = true;
                    }
                }
            }
            jet1PrevCount = targetCount;
        }
        
        // ---------------- Jet2 IFF ----------------
        if (jet2IFF !is null)
        {
            int targetCount = jet2IFF.getIFFTargetCount();
            if (targetCount > jet2PrevCount)
            {
                for (int i = jet2PrevCount; i < targetCount; i++)
                {
                    string responderId, responderName, mode, code;
                    float distance, angle;
                    int status;
                    
                    if (jet2IFF.getIFFTarget(i, responderId, responderName, mode, code, distance, angle, status))
                    {
                        string statusStr = (status == 1) ? "Friend" : "Foe";
                        
                        Print("[Jet2IFF] 🎯 New IFF contact detected #" + i 
                            + " | ID: " + responderId
                            + " | Name: " + responderName
                            + " | Status: " + statusStr
                            + " | Mode: " + mode
                            + " | Code: " + code
                            + " | Distance: " + distance + " km"
                            + " | Angle: " + angle + "°");
                        
                        // 🎯 Automatically switch to Jet2 display on detection
                        e.selectEntityDisplay(jet2IFF);
                        Print("📡 Switched to Jet2 IFF display");
                        
                        // Wait for display update
                        e.sleep(1000);
                        
                        // 📸 Capture DUAL screenshots (IFF Display + Canvas)
                        e.captureSensorScreenshot("jet2_iff_detection.png");
                        Print("📸 Jet2 IFF detection screenshots captured!");
                        Print("   → IFF display screenshot saved");
                        Print("   → Tactical canvas screenshot saved");
                        
                        detectedByJet = "Jet2";
                        simulationStopped = true;
                    }
                }
            }
            jet2PrevCount = targetCount;
        }
        
        // Exit loop immediately if any target detected
        if (simulationStopped)
        {
            Print("🛑 IFF contact detected by " + detectedByJet + "! Stopping simulation at t = " + t + " seconds.");
            Print("📊 Generating PDF report...");
            e.generatePDFReport("IFF_Detection_Report.pdf");
            Print("✅ Report generated: IFF_Detection_Report.pdf");
            Print("📄 Report includes:");
            Print("   - Initial IFF display screenshot");
            Print("   - Initial tactical canvas screenshot");
            Print("   - Detection IFF display screenshot");
            Print("   - Detection tactical canvas screenshot");
            Print("   - Complete detection logs");
            break;
        }
    }
    
    SimPause(e);
    Print("✅ Simulation finished!");
    Print("📋 Summary:");
    Print("   - Jet1 detected " + jet1PrevCount + " IFF contact(s)");
    Print("   - Jet2 detected " + jet2PrevCount + " IFF contact(s)");
    Print("🎯 Check the PDF for complete visual evidence!");
}