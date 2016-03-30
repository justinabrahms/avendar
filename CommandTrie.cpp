#include "CommandTrie.h"
using namespace std;

const int CommandTrie::NoIndex(-1);

CommandTrie::Node::~Node()
{
    for (MapType::iterator iter(children.begin()); iter != children.end(); ++iter)
        delete iter->second;
}

/// @return: true if added, false if already present
bool CommandTrie::Add(const char * command, int index, int trust)
{
    Node * currNode(m_root);
    for (size_t i(0); command[i] != '\0'; ++i)
    {
        // Lookup the appropriate child node
        MapType::iterator iter(currNode->children.find(command[i]));
        if (iter != currNode->children.end())
        {
            // Advance the node
            currNode = iter->second;
            continue;
        }

        // Reached the end of existing nodes, so spawn all the remaining ones
        while (command[i] != '\0')
        {
            // Make a new child and add it
            Node * child(new Node);
            currNode->children.insert(std::make_pair(command[i], child));
            currNode = child;
            ++i;
        }
        break;
    }

    // Found a node with this command (or possibly just added it); check for overwrite
    if (currNode->index == NoIndex)
    {
        // This must have been a new node or a stop on the way to another command, so fill it in
        currNode->index = index;
        currNode->trust = trust;
        return true;
    }

    // Command already exists here
    return false;
}

int CommandTrie::Lookup(const char * command, int trust) const
{
    const Node * currNode(m_root);
    for (size_t i(0); command[i] != '\0'; ++i)
    {
        // Lookup the appropriate child node, bailing if not found
        MapType::const_iterator iter(currNode->children.find(command[i]));
        if (iter == currNode->children.end())
            return NoIndex;

        // Advance to the next node
        currNode = iter->second;
    }

    // Reached the end of the command, so take the lowest-indexed acceptable child node
    return FindLowestInChildren(currNode, trust);
}

int CommandTrie::FindLowestInChildren(const Node * node, int trust) const
{
    // Determine this node's index, then walk down the children as well for the lowest
    int index(trust >= node->trust ? node->index : NoIndex);
    for (MapType::const_iterator iter(node->children.begin()); iter != node->children.end(); ++iter)
    {
        // Find the lowest index of the children
        int childIndex(FindLowestInChildren(iter->second, trust));
        if (index == NoIndex || (childIndex != NoIndex && childIndex < index))
            index = childIndex;
    }

    return index;
}
