import java.util.*;
import java.awt.*;

class ProbeList
extends java.awt.List
{

private int              count;
private java.util.Vector info;
private String           error;

public ProbeList()
    {
        count = 0;
        error = null;
        setVisible(true);
    }

private String getProbeInfo(int index) {
        if (index<0 || index >= count) return null;
        return (String)info.elementAt(index);
    }

private void rebuildList()
    {
//         setVisible(false);
        clear();
        if (count>0) {
            for (int i = 0; i<count; i++) {
                addItem(getProbeInfo(i));
            }
        }
        else {
            addItem("None found.");
        }
//         setVisible(true);
    }

public void setContents(ServerAnswer parsed) {
        if (parsed.hasError()) {
            error = parsed.getError();
        }
        else {
            error        = null;
            String found = parsed.getValue("found");
            count        = Integer.parseInt(found);

            info = new Vector(count);

            for (int i = 0; i<count; i++) {
                String probe = parsed.getValue("probe"+i);
                System.out.println("probe='"+probe+"' index="+i);
                info.addElement(probe);
            }
        }

        rebuildList();
    }

public boolean hasError()
    {
        return error != null;
    }

public String getError()
    {
        return error;
    }

// public void paint()
//     {
//     }

}
