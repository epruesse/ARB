import java.util.*;
import java.awt.*;

class ProbeList
    extends java.awt.List
{
    private int        count  = 0;
    private NodeProbes probes = null;
    private String     error  = null;

    // private java.util.Vector info;
    // private int              preferredHeight;
    // private int              preferredWidth;

    // public ProbeList(int width, int height)
    public ProbeList()
    {
        //         preferredHeight = height;
        //         preferredWidth  = width;
//         count                      = 0;
//         error                      = null;
//         probes                     = null;
        setVisible(true);
    }

    public int length() { return count; }

    public Probe getProbe(int index) {
        if (probes == null) return null;
        return probes.getProbe(index);
    }

    private void rebuildList()
    {
        removeAll();
        if (count>0) {
            for (int i = 0; i<count; i++) {
                Probe probe = getProbe(i);

                if (probe != null) {
                    add(probe.getDisplayString());
                }
                else {
                    add("--- Error: No probe! ---");
                }

//                 String pinfo = getProbeInfo(i);
//                 int    komma = pinfo.indexOf(',');
//
//                 if (komma != -1) add(pinfo.substring(0, komma));
//                 else add(pinfo);
            }
        }
        else {
            add("No probes found.");
        }
    }

    public void setContents(NodeProbes nodeProbes) throws Exception {
        probes = nodeProbes;
        count  = probes.size();
        rebuildList();
    }

    //     public void setContents(ServerAnswer parsed) throws Exception
    //     {
    //         if (parsed.hasError()) {
    //             error = parsed.getError();
    //         }
    //         else {
    //             error        = null;
    //             String found = parsed.getValue("found");
    //             count        = Integer.parseInt(found);

    //             info = new Vector(count);

    //             for (int i = 0; i<count; i++) {
    //                 String probe = parsed.getValue("probe"+i);
    //                 // System.out.println("probe='"+probe+"' index="+i);
    //                 info.addElement(probe);
    //             }
    //         }

    //         rebuildList();
    //     }

    public boolean hasError()
    {
        return error != null;
    }

    public String getError()
    {
        return error;
    }

    // overloading of java.awt.component method, is called from layout manager to determine optimal size of widget
    // public Dimension getPreferredSize()
    //     {
    //         return new Dimension(preferredWidth, preferredHeight);
    //     }


}
