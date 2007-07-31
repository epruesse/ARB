//  ==================================================================== // 
//                                                                       // 
//    File      : NodeProbes.java                                        // 
//    Purpose   : Contains probes for one node                           // 
//    Time-stamp: <Thu Mar/18/2004 17:49 MET Coder@ReallySoft.de>        // 
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
    private Vector probes;
    private Vector sorted_probes;

    public static HttpSubsystem webAccess;

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
                    }
                }
                catch (ClientException e) {
                    error = e.get_plain_message();
                }
            }
        }

        if (error != null)
            Toolkit.AbortWithError("While retrieving probe information: "+error);

        resortProbes();
    }

    public void resortProbes() {
        TreeSet sorted = new TreeSet(probes);
        sorted_probes  = new Vector(sorted);
    }

    Probe getProbe(int index) {
        if (index < 0 || index >= sorted_probes.size()) return null;
        return (Probe)sorted_probes.get(index);
    }
    int getSortedIndexOf(Probe p) {
        return sorted_probes.indexOf(p);
    }
    int getSortedIndexOf(String probeString) {
        int size = sorted_probes.size();
        for (int i = 0; i<size; ++i) {
            Probe p = (Probe)sorted_probes.get(i);
            if (p.sequence().equals(probeString)) {
                return i;
            }
        }
        return -1;
    }

    int size() {
        return sorted_probes.size();
    }

    public void cacheAllHits() throws Exception {
        Toolkit.showMessage("Caching hits for all listed probes:");
        int size = probes.size();
        for (int i = 0; i<size; ++i) {
            String dummy = ((Probe)probes.get(i)).members();
        }
        Toolkit.showMessage("Ready.");
    }
}
