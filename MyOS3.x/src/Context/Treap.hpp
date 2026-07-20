// Template implementation, included from Treap.h

/**
 * Implements an unbalanced binary search tree.
 * Note that all "matching" is based on the compares method.
 * @author Mark Allen Weiss
 */
/**
 * Construct the treap.
 */
template<class Comparable> Treap<Comparable>::Treap(const Comparable & notFound) :
    ITEM_NOT_FOUND(notFound )
{
    nullNode = new TreapNode<Comparable>;
    nullNode->left = nullNode->right = nullNode;
    nullNode->priority = INT_MAX;
    root = nullNode;
}

/**
 * Copy constructor.
 */
template<class Comparable> Treap<Comparable>::Treap(
        const Treap<Comparable> & rhs) :
    ITEM_NOT_FOUND(rhs.ITEM_NOT_FOUND)
{
    nullNode = new TreapNode<Comparable>;
    nullNode->left = nullNode->right = nullNode;
    nullNode->priority = INT_MAX;
    root = nullNode;
    *this = rhs;
}

/**
 * Destructor for the tree.
 */
template<class Comparable> Treap<Comparable>::~Treap()
{
    makeEmpty();
    delete nullNode;
}

/**
 * Insert x into the tree; duplicates are ignored.
 */
template<class Comparable> bool Treap<Comparable>::insert(const Comparable & x) throw()
{
    return insert(x, root );
}

/**
 * Remove x from the tree. Nothing is done if x is not found.
 */
template<class Comparable> Comparable Treap<Comparable>::remove(
        const Comparable & x)
{
    return remove(x, root );
}

/**
 * Find the smallest item in the tree.
 * Return smallest item or ITEM_NOT_FOUND if empty.
 */
template<class Comparable> const Comparable & Treap<Comparable>::findMin() const
{
    if (isEmpty() )
        return ITEM_NOT_FOUND;

    TreapNode<Comparable> *ptr = root;
    while (ptr->left != nullNode )
        ptr = ptr->left;

    return ptr->element;
}

/**
 * Find the largest item in the tree.
 * Return the largest item of ITEM_NOT_FOUND if empty.
 */
template<class Comparable> const Comparable & Treap<Comparable>::findMax() const
{
    if (isEmpty() )
        return ITEM_NOT_FOUND;

    TreapNode<Comparable> *ptr = root;
    while (ptr->right != nullNode )
        ptr = ptr->right;

    return ptr->element;
}

/**
 * Find item x in the tree.
 * Return the matching item or ITEM_NOT_FOUND if not found.
 *
 template <class Comparable>
 const Comparable & Treap<Comparable>::
 find( const Comparable & x ) const
 {
 TreapNode<Comparable> *current = root;
 nullNode->element = x;

 for( ; ; )
 {
 if( x < current->element )
 current = current->left;
 else if( current->element < x )
 current = current->right;
 else if( current != nullNode )
 return current->element;
 else
 return ITEM_NOT_FOUND;
 }
 }
 */

/**
 * Find item x in the tree.
 * Return the matching item or ITEM_NOT_FOUND if not found.
 */
template<class Comparable> const Comparable & Treap<Comparable>::findFrom(
        const Comparable & x, TreapNode<Comparable> *from) const
{
    TreapNode<Comparable> *current = from; // JVB: changed from 'root'
    nullNode->element = x;

    for (;;)
    {
        if( x < current->element )
        current = current->left;
        else if( current->element < x )
        current = current->right;
        else if( current != nullNode )
        return current->element;
        else
        return ITEM_NOT_FOUND;
    }
}

template<class Comparable> inline const Comparable &Treap<Comparable>::findFirst(
        const Comparable & x) const
{
    return findFrom(x, root );
}

template<class Comparable> inline const Comparable &Treap<Comparable>::findNext(
        const Comparable & x, const Comparable& start) const
{
    // direct cast back to the containing TreapNode...
    TreapNode<Comparable> *at = (TreapNode<Comparable> *) &start;

    return findFrom(x, at->right);
}

/**
 * Make the tree logically empty.
 */
template<class Comparable> void Treap<Comparable>::makeEmpty()
{
    makeEmpty(root )
;}

/**
 * Test if the tree is logically empty.
 * Return true if empty, false otherwise.
 */
template<class Comparable> inline bool Treap<Comparable>::isEmpty() const
{
    return root == nullNode;
}

/**
 * Perform function f on the tree in sorted order.
 *
 * return sum(f) over all nodes, -1 if empty
 */
template<class Comparable> int Treap<Comparable>::foreach(nodefunction f,
        void* param) const
{
    if ( !isEmpty() )
        return execute(root, f, param );
    else
        return -1;
}

/**
 * Deep copy.
 */
template<class Comparable> const Treap<Comparable> &Treap<Comparable>::operator=( 
        const Treap<Comparable> & rhs )
{
    if( this != &rhs )
    {
        makeEmpty( );
        root = clone( rhs.root );
    }

    return *this;
}

/**
 * Internal method to insert into a subtree.
 * x is the item to insert.
 * t is the node that roots the tree.
 * Set the new root.
 */
template<class Comparable> bool Treap<Comparable>::insert(const Comparable & x,
        TreapNode<Comparable> * & t)
{
    if (t == nullNode )
        t = new TreapNode<Comparable>( x, nullNode, nullNode,
                randomNums.randomInt( ) )
;    else if( x < t->element )
    {
        insert( x, t->left );
        if( t->left->priority < t->priority )
        rotateWithLeftChild( t );
    }
    else if( t->element < x )
    {
        insert( x, t->right );
        if( t->right->priority < t->priority )
        rotateWithRightChild( t );
    }
    else
    {
        // duplicate; do nothing
        return false;
    }
    return true; // OK, inserted
}

/**
 * Internal method to remove from a subtree.
 * x is the item to remove.
 * t is the node that roots the tree.
 * Set the new root.
 */
template<class Comparable> Comparable Treap<Comparable>::remove(
        const Comparable & x, TreapNode<Comparable> * & t)
{
    if (t != nullNode )
    {
        if (x < t->element)
            return remove(x, t->left);
        else if (t->element < x )
            return remove(x, t->right);
        else
        {
            // Match found
            if (t->left->priority < t->right->priority)
                rotateWithLeftChild(t )
;            else
            rotateWithRightChild( t );

            if( t != nullNode ) // Continue on down
            return remove( x, t );
            else
            {
                delete t->left;
                t->left = nullNode; // At a leaf
                return t->element;
            }
        }
    }
    else return nullNode->element;
}

/**
 * Internal method to make subtree empty.
 */
template<class Comparable> void Treap<Comparable>::makeEmpty(
        TreapNode<Comparable> * & t)
{
    if (t != nullNode )
    {
        makeEmpty(t->left)
;
                        makeEmpty(t->right)
;
                        delete t
;}
t = nullNode;
}

/**
 * Internal method to traverse a subtree in sorted order.
 * @param f the function to perform on each node
 * @param t the node that roots the tree.
 * @param param the parameter to pass to the function
 *
 * @return Sum of values obtained from f
 */
template<class Comparable> int Treap<Comparable>::execute(
        TreapNode<Comparable> *t, nodefunction f, void* param) const
{
    if (t != nullNode )
    {
        int r = execute(t->left, f, param );
        r += ((t->element).*f)(param);
        r += execute(t->right, f, param );
        return r;
    }
    return 0;
}

/**
 * Internal method to clone subtree.
 */
template<class Comparable> TreapNode<Comparable> *Treap<Comparable>::clone(
        TreapNode<Comparable> * t) const
{
    if (t == t->left) // Cannot test against nullNode!!!
        return nullNode;
    else
        return new TreapNode<Comparable>( t->element, clone( t->left ),
                clone( t->right ), t->priority );
}

/**
 * Rotate binary tree node with left child.
 */
template<class Comparable> void Treap<Comparable>::rotateWithLeftChild(
        TreapNode<Comparable> * & k2) const
{
    TreapNode<Comparable> *k1 = k2->left;
    k2->left = k1->right;
    k1->right = k2;
    k2 = k1;
}

/**
 * Rotate binary tree node with right child.
 */
template<class Comparable> void Treap<Comparable>::rotateWithRightChild(
        TreapNode<Comparable> * & k1) const
{
    TreapNode<Comparable> *k2 = k1->right;
    k1->right = k2->left;
    k2->left = k1;
    k1 = k2;
}
