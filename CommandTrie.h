#ifndef COMMANDTRIE_H
#define COMMANDTRIE_H

#include <map>

class CommandTrie
{
    public:
        static const int NoIndex;

        CommandTrie();
        ~CommandTrie();

        bool Add(const char * command, int index, int trust);
        int Lookup(const char * command, int trust) const;

    private:
        // Types
        class Node;
        typedef std::map<char, Node*> MapType;

        class Node
        {
            public:
                Node();
                ~Node();

                MapType children;
                int index;
                int trust;

            private:
                Node(const Node &);
                void operator=(const Node &);
        };

        // Methods 
        CommandTrie(const CommandTrie &);       // Disable copying
        void operator=(const CommandTrie &);    // Disable copying
        int FindLowestInChildren(const Node * node, int trust) const;

        // Members
        Node * m_root;
};

inline CommandTrie::CommandTrie() : m_root(new Node) {}
inline CommandTrie::~CommandTrie() {delete m_root;}
inline CommandTrie::Node::Node() : index(NoIndex), trust(0) {}

#endif
