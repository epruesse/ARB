// class IOManager
// encapsulates modal file dialogs for
// reading and saving of configuration settings and result
// coded by Lothar Richter March 2004
// (c) ARB-Project


import java.awt.*;
import java.io.*;

public class IOManager{
    private String configFile = new String(".arbprobeconfig");
    private String detailFileName;
    private String probeFileName;
    private Frame gui;

    public IOManager(Frame controlledGui, String defaultConfigFile){
	if (defaultConfigFile.equals("")) defaultConfigFile = ".arbprobeconfig";
	gui = controlledGui;
    }


    public boolean readConfiguration(){return false;};
    public boolean saveConfiguration(){return false;};
    public boolean saveResults(String destination, String results){
	try {

	    if (destination.equals("details")){
		if(detailFileName == null){ return saveResultsAs(destination, results);}
		else{
		    FileWriter outfile = new FileWriter(detailFileName);
		    outfile.write(results);
		    outfile.flush();
		    outfile.close();
		    return true;
		}
	    }else{
		
		if (destination.equals("probe list")){
		    if(probeFileName == null){ return saveResultsAs(destination, results);}
		    else{
			FileWriter outfile = new FileWriter(probeFileName);
			outfile.write(results);
			outfile.flush();
			outfile.close();
			return true;
		    }
		}else{

		    if (destination.equals("config")){
			if(configFile == null){ return saveResultsAs(destination, results);}
			else{
			    FileWriter outfile = new FileWriter(configFile);
			    outfile.write(results);
			    outfile.flush();
			    outfile.close();
			    return true;
			}
		    }else{
			System.out.println("IOManager:Unrecognized result type"); return false;}
		}
	    }
	}catch(IOException e){
	    System.out.println("IOManager:error while try to save results");
	    return false;
	}
	
	
	
    }
    public boolean saveResultsAs(String destination, String results){

	FileDialog fd = new FileDialog(gui,new String("Save " + destination + " as"),FileDialog.SAVE);
	fd.setVisible(true);
	if (!(fd.getFile()== null)){
	if (destination.equals("details")) detailFileName = fd.getDirectory() + fd.getFile();
	if (destination.equals("probe list")) probeFileName = fd.getDirectory() + fd.getFile();
	System.out.println(fd.getFile());
	System.out.println(fd.getDirectory());
	return saveResults(destination, results);
	}else{
	    return false;
	}

    }



}
