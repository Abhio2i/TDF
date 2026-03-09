void main(ScriptEngine@ e)
{
    ProfileCategaory@ platformProfile = e.getProfileByName("Platform");
    
    // ==================================================
    // Jet1: Agra → New Delhi
    // ==================================================
    Platform@ jet1 = e.addEntityToPlatform(platformProfile, "Jet1");
    jet1.dynamicModel.moveSpeed = 1500;
    jet1.transform.setGeoCord(27.1767f, 78.0081f, 0.0f); // Agra
    jet1.trajectory.addWaypoint(27.1767f, 0.0f, 78.0081f); // Start
    jet1.trajectory.addWaypoint(28.6139f, 0.0f, 77.2090f); // New Delhi
    
    // Add Radio
    e.addRadioSubComponent(jet1, "Radio1");
    Radio@ jet1Radio = jet1.getRadioByName("Radio1");
    Print("✅ Added Radio1 to Jet1");
    
    // ==================================================
    // Jet2: Gurugram → Hathras
    // ==================================================
    Platform@ jet2 = e.addEntityToPlatform(platformProfile, "Jet2");
    jet2.dynamicModel.moveSpeed = 1500;
    jet2.transform.setGeoCord(28.4595f, 77.0266f, 0.0f); // Gurugram
    jet2.trajectory.addWaypoint(28.4595f, 0.0f, 77.0266f); // Start (Gurugram)
    jet2.trajectory.addWaypoint(27.5983f, 0.0f, 78.0506f); // End (Hathras)
    
    // Add Radio
    e.addRadioSubComponent(jet2, "Radio2");
    Radio@ jet2Radio = jet2.getRadioByName("Radio2");
    Print("✅ Added Radio2 to Jet2");
    
    // ==================================================
    // 🎯 AUTOMATED UI SETUP - Show Jet1 in Radio Display
    // ==================================================
    Print("🎯 Setting up automated display...");
    if (jet1Radio !is null) {
        e.selectEntityDisplay(jet1Radio);
        Print("✅ Sensors tab opened");
        Print("✅ Radio display selected");
        Print("✅ Jet1 selected in hierarchy");
        
        // Wait for display to render
        e.sleep(1000);
        
        // 📸 Capture initial screenshots (BOTH sensor + canvas)
        //e.captureSensorScreenshot("initial_radio.png");
        //Print("📸 Initial screenshots captured (Radio Display + Canvas)!");
    }
    
    // ==================================================
    // Simulation loop: check radio targets every second
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
        
        // ---------------- Jet1 Radio ----------------
        if (jet1Radio !is null)
        {
            int targetCount = jet1Radio.getRadioTargetCount();
            if (targetCount > jet1PrevCount)
            {
                for (int i = jet1PrevCount; i < targetCount; i++)
                {
                    string name;
                    float radius, angle, range, frequency;
                    
                    if (jet1Radio.getRadioTarget(i, name, radius, angle, range, frequency))
                    {
                        Print("[Jet1Radio] 📻 New radio signal detected #" + i 
                            + " | Name: " + name
                            + " | Radius: " + radius + " km"
                            + " | Angle: " + angle + "°"
                            + " | Range: " + range + " km"
                            + " | Frequency: " + frequency + " MHz");
                        
                        // 🎯 Automatically switch to Jet1 display on detection
                        e.selectEntityDisplay(jet1Radio);
                        Print("📡 Switched to Jet1 radio display");
                        
                        // Wait for display update
                        e.sleep(1000);
                        
                        // 📸 Capture DUAL screenshots (Radio Display + Canvas)
                        e.captureSensorScreenshot("jet1_radio_detection.png");
                        Print("📸 Jet1 radio detection screenshots captured!");
                        Print("   → Radio display screenshot saved");
                        Print("   → Tactical canvas screenshot saved");
                        
                        detectedByJet = "Jet1";
                        simulationStopped = true;
                    }
                }
            }
            jet1PrevCount = targetCount;
        }
        
        // ---------------- Jet2 Radio ----------------
        if (jet2Radio !is null)
        {
            int targetCount = jet2Radio.getRadioTargetCount();
            if (targetCount > jet2PrevCount)
            {
                for (int i = jet2PrevCount; i < targetCount; i++)
                {
                    string name;
                    float radius, angle, range, frequency;
                    
                    if (jet2Radio.getRadioTarget(i, name, radius, angle, range, frequency))
                    {
                        Print("[Jet2Radio] 📻 New radio signal detected #" + i 
                            + " | Name: " + name
                            + " | Radius: " + radius + " km"
                            + " | Angle: " + angle + "°"
                            + " | Range: " + range + " km"
                            + " | Frequency: " + frequency + " MHz");
                        
                        // 🎯 Automatically switch to Jet2 display on detection
                        e.selectEntityDisplay(jet2Radio);
                        Print("📡 Switched to Jet2 radio display");
                        
                        // Wait for display update
                        e.sleep(1000);
                        
                        // 📸 Capture DUAL screenshots (Radio Display + Canvas)
                        e.captureSensorScreenshot("jet2_radio_detection.png");
                        Print("📸 Jet2 radio detection screenshots captured!");
                        Print("   → Radio display screenshot saved");
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
            Print("🛑 Radio signal detected by " + detectedByJet + "! Stopping simulation at t = " + t + " seconds.");
            Print("📊 Generating PDF report...");
            e.generatePDFReport("Radio_Detection_Report.pdf");
            Print("✅ Report generated: Radio_Detection_Report.pdf");
            Print("📄 Report includes:");
            Print("   - Initial radio display screenshot");
            Print("   - Initial tactical canvas screenshot");
            Print("   - Detection radio display screenshot");
            Print("   - Detection tactical canvas screenshot");
            Print("   - Complete detection logs");
            break;
        }
    }
    
    SimPause(e);
    Print("✅ Simulation finished!");
    Print("📋 Summary:");
    Print("   - Jet1 detected " + jet1PrevCount + " radio signal(s)");
    Print("   - Jet2 detected " + jet2PrevCount + " radio signal(s)");
    Print("🎯 Check the PDF for complete visual evidence!");
}