//  ==================================================================== // 
//                                                                       // 
//    File      : NodeProbes.java                                        // 
//    Purpose   : Contains probes for one node                           // 
//    Time-stamp: <Fri Mar/05/2004 21:42 MET Coder@ReallySoft.de>        // 
//                                                                       // 
//                                                                       // 
//  Coded by Ralf Westram (coder@reallysoft.de) in March 2004            // 
//  Copyright Department of Microbiology (Technical University Munich)   // 
//                                                                       // 
//  Visit our web site at: http://www.arb-home.de/                       // 
//                                                                       // 
//  ==================================================================== //

import java.util.*;

public class NodeProbes
{
    private Vector       probes = null;
    public static HttpSubsystem webAccess = null;

    public NodeProbes(TreeNode target, boolean exactMatches) throws Exception {
        String encodedPath = target.getCodedPath();
        String answer      = webAccess.retrieveNodeInformation(encodedPath, exactMatches);
        String error       = null;

        if (answer == null) {
            error = webAccess.getLastRequestError();
        }
        else {
            ServerAnswer parsed = new ServerAnswer(answer, true, false);
            if (parsed.hasError()) {
                error = parsed.getError();
            }
            else {
                String found = parsed.getValue("found");
                int    count = Integer.parseInt(found);
                probes       = new Vector(count);

                try {
                    for (int i = 0; i<count; ++i) {
                        String probe_def = parsed.getValue("probe"+i);
                        probes.addElement(new Probe(probe_def, target));

                        //                     int komma = probe.indexOf(',');

                        //                     if (komma == -1) {
                        //                         error = "comma expected in '"+probe+"'";
                        //                         break;
                        //                     }

                        //                     probes.addElement(new Probe(probe.substring(komma+1), probe.substring(0, komma)));
                    }
                }
                catch (ClientException e) {
                    error = e.get_plain_message();
                }
            }
        }

        if (error != null)
            Toolkit.AbortWithError("While retrieving probe information: "+error);
    }

    Probe getProbe(int index) {
        if (index < 0 || index >= probes.size()) {
            return null;
        }
        return (Probe)probes.get(index);
    }

    int size() {
        return probes.size();
    }
}
