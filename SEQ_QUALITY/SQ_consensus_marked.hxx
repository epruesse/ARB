class SQ_consensus_marked {

public:
    SQ_consensus_marked();
    void SQ_clac_consensus(char& base, int& position);

private:
    char consensus[40000][1];
    char base;
    int position;

};

