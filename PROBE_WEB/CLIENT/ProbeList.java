import java.util.*;
import java.awt.*;

class ProbeList
extends java.awt.List
{

private int probeWidth = 150;

private int              count;
private java.util.Vector info;
private String           error;
private int preferredHeight;
private int preferredWidth;

public ProbeList(int width, int height)
    {

        preferredHeight = height;
        preferredWidth = width;
        count = 0;
        error = null;
     //    Dimension dim = getSize();
//         System.out.println("width: " + dim.width);
//         dim.width = probeWidth;
//         System.out.println("width: " + dim.width);
//         //        setSize(dim);
//         doLayout();
        setVisible(true);
    }

public String getProbeInfo(int index) {
        if (index<0 || index >= count) return null;
        return (String)info.elementAt(index);
    }

private void rebuildList()
    {
//         setVisible(false);
//        clear();
        removeAll();
        if (count>0) {
            for (int i = 0; i<count; i++) {
                add(getProbeInfo(i));
            }
        }
        else {
            add("None found.");
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


// overloading of java.awt.component method, is called from layout manager to determine optimal size of widget
public Dimension getPreferredSize()
    {
        return new Dimension(preferredWidth, preferredHeight);

    }


}
