%module ARB

%exception open {
    $action
    if (!result) {
        PyErr_SetString(PyExc_Exception, await_error());
        return NULL;
    }
}

%include ../ARB.i

########## Python side interface code ########
#
# These are just some wrappers that make the ARB API more
# easily accessible within the typical object oriented python
# style

%pythoncode %{

class Species(object):
    """Wraps GBDATA to make accesssing data fields easier"""
    def __init__(self, gbd):
        self.gbd=gbd
    def __getattr__(self, field):
        return read_as_string(self.gbd, field)

class ArbDB(object):
    """Wraps "GBMAIN" """
    def __init__(self, gbd):
        self.gbmain=gbd
    def close(self):
        return close(self.gbmain)
    def save_as(filename, mode="b"):
        return save_as(self.gbmain, filename, mode)
    def marked_species(self):
        """Iterates through all marked species"""
        curr = first_marked_species(self.gbmain)
        while curr:
            yield curr
            curr = next_marked_species(curr)
    def all_species(self):
        """Iterates through all species"""
        curr = first_species(self.gbmain)
        while curr:
            yield Species(curr)
            curr = next_species(curr)

def open(filename, mode="rw"):
    return ArbDB(_ARB.open(filename, mode))

%}

######## End of Python side interface code ######

