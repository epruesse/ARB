typedef unsigned char boolean;

#define FALSE 0
#define TRUE  1


class Element {
    
private:
    int contents;               // contents of the elment

    boolean         marked;

    class Element   *first_son;     //
    class Element   *last_son;      //
    class Element   *left_brother;  //
    class Element   *right_brother; //

    int             width;          // my own width
    int             height;         // my own height
    int             width_of_sons;  // width of my sons
    int             height_of_sons; // height of my sons


    int             number_of_sons; // number of sons of the element
public:
    Element();
    ~Element();
    class Element   *set_contents(int v);
    class Element   *get_contents(int &v);
    class Element   *print_contents(void);
    class Element   *print_contents_recursive(int depth);
    class Element   *dump(void);
    class Element   *dump_recursive();

    class Element   *calculate_sizes(void);

    class Element   *add_son(void);
    class Element   *get_number_of_sons(int &n);
    class Element   **get_sons(void);
};
