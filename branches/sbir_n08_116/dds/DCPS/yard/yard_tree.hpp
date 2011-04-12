// Public domain, by Christopher Diggins
// http://www.cdiggins.com
//
// This files is for the construction of parse trees. Every AbstractNode in a parse tree is derived 
// from AbstractNode and provides access to the rule it represents through a function:
// virtual const type_info& GetRule(). Children are arranged by type. 
// Once you have the first child AbstractNode, you can repeatedly call GetSibling() to get the 
// next AbstractNode of the same type. 
//
// Parse trees are created automatically by inserting Store<T> productions into a grammar. 

#ifndef YARD_TREE_HPP
#define YARD_TREE_HPP

#include <typeinfo>

namespace yard
{   
    /////////////////////////////////////////////////////////////////////
    // Abstract Syntax Tree 

    // The AbstractNodes in a ParseTree are usually generated by adding Store<Rule_T> production rules        
    // to the grammar. The parser then responds by construction AbstractNodes in this tree.
    // The Ast class contains Ast::AbstractNode objects which contain a type_info struct to identify
    // the rule type (i.e. the Rule_T) that they are associated with.
    // The template parameter "Iter_T" is the iterator associated with the input.
    template<typename Iter_T = const char*>
    struct Ast
    {
        // constructor
        Ast() 
            : current(NULL), root(NULL, NULL)
        { 
            current = &root;
        }

#ifdef OPENDDS_USE_NESTED_TEMPLATE_FWDECL
        // a forward declaration
        template<typename T>
        struct TypedNode;
#endif

        // a node of the tree
        struct AbstractNode
        {
            // public types 
            typedef Iter_T TokenIter;     
                
            // constructor, note that this uses staged construction
            // the node is not completely constructed until the children-nodes 
            // are manually added and Complete(...) is called
            AbstractNode(TokenIter pos, AbstractNode* parent) 
            {  
                mpFirst = pos;
                mpLast = pos;
                mpNext = NULL;
                mpChild = NULL;
                mpLastChildPtr = &mpChild;
                mpParent = parent;
                mbCompleted = false;
            }            

            // destructor
            virtual ~AbstractNode()
            {
                if (mpNext)
                    delete mpNext;
                if (mpChild)
                    delete mpChild;
            }

            // adds a new abstract child node to the tree
            void AddChild(AbstractNode* child)
            {
                assert(!IsCompleted());
                assert(mpNext == NULL);
                assert(*mpLastChildPtr == NULL);
                *mpLastChildPtr = child;
                mpLastChildPtr = &(child->mpNext);
            }    

#ifdef OPENDDS_USE_NESTED_TEMPLATE_FWDECL
            // adds a new concrete type node to the tree associated 
            // with a specific rule
            template<typename Rule_T>
            TypedNode<Rule_T>* NewChild(TokenIter pos)
            {
                //printf("%s\n", typeid(Rule_T).name());
                assert(!IsCompleted());
                TypedNode<Rule_T>* ret = new TypedNode<Rule_T>(pos, this);
                AddChild(ret);
                return ret;
            }  
#endif

            // delete a child node 
            void DeleteChild(AbstractNode* p)
            {
                // This function only allows deletion of the last child
                // in a list.
                assert(p->mpNext == NULL);
                assert(&(p->mpNext) == mpLastChildPtr);
                assert(mpChild != NULL);

                if (p == mpChild) 
                {
                    delete p;
                    mpChild = NULL;
                    mpLastChildPtr = &mpChild;                
                }
                else
                {
                    // Start at first child
                    AbstractNode* pBeforeLast = mpChild;                    

                    // iterate through siblings, until we reach the child before 
                    // the last one
                    while (pBeforeLast->mpNext != p)
                    {
                        pBeforeLast = pBeforeLast->mpNext;
                        assert(pBeforeLast != NULL);
                    }
                    delete p;
                    pBeforeLast->mpNext = NULL;
                    mpLastChildPtr = &(pBeforeLast->mpNext);                
                }
            }

            // returns true iff there are any child nodes
            bool HasChildren() 
            {
                return mpChild != NULL;
            }

            // returns a pointer to the first child node
            AbstractNode* GetFirstChild()
            {
                return mpChild;
            }

            // returns true if this node has a sibling
            bool HasSibling() 
            {
                return mpNext != NULL;
            }

            // gets a pointer to the sbiling node
            AbstractNode* GetSibling()
            {
                return mpNext;
            }

            // return true if the associated rule type matches the template parameter
            template<typename T>
            bool TypeMatches()
            {
                return GetRuleTypeInfo() == typeid(T);
            }

#ifdef OPENDDS_USE_NESTED_TEMPLATE_FWDECL
            // returns a pointer to the first sibling node that matches the rule
            template<typename T>
            TypedNode<T>* GetTypedSibling()
            {
                AbstractNode* sib = GetSibling();
                while (sib != NULL)
                {
                    if (sib->TypeMatches<T>())
                        return dynamic_cast<TypedNode<T>*>(sib);
                    sib = sib->GetSibling();
                }
                return NULL;
            }

            // returns a pointer to the first child node that matches the rule 
            template<typename T>
            TypedNode<T>* GetFirstTypedChild()
            {
                AbstractNode* ret = GetFirstChild();
                if (ret == NULL) return NULL;
                if (ret->TypeMatches<T>()) return dynamic_cast<TypedNode<T>*>(ret);
                return ret->GetTypedSibling<T>();
            }
#endif
            // gets a pointer to the parent pointer of the node
            AbstractNode* GetParent()
            {
                return mpParent;
            }

            // this is called as part of a multi-stage 
            void Complete(TokenIter pos) {      
                assert(!IsCompleted());
                mpLast = pos;      
                mbCompleted = true;
                assert(IsCompleted());
            }

            // returns true if the node was completely created, 
            // nodes may be destroyed before they are completely created.
            bool IsCompleted() {
                return mbCompleted;
            }               

            // returns the beginning of the input associated with the node 
            TokenIter GetFirstToken() {
                assert(IsCompleted());
                return mpFirst;
            }    

            // returns the end of the input associated with the node
            TokenIter GetLastToken() {
                assert(IsCompleted());
                return mpLast;
            }

            // calls the function "F" with each child node 
            template<typename F>
            void ForEach(F f)
            {
                for (AbstractNode* child = GetFirstChild();
                    child != NULL; 
                    child = child->GetSibling())
                    f(child);
            }

#ifdef OPENDDS_USE_NESTED_TEMPLATE_FWDECL
            // calls the function "F" with each child node associated with the rule type
            template<typename Rule_T, typename F>
            void ForEachTyped(F f)
            {
                for (AbstractNode* child = GetFirstTypedChild<Rule_T>();
                    child != NULL; 
                    child = child->GetTypedSibling<Rule_T>())
                    f(child);
            }
#endif
            // abstract member functions
            virtual const std::type_info& GetRuleTypeInfo() = 0;
            
        private:

            // fields
            bool mbCompleted;
            AbstractNode* mpParent;
            AbstractNode* mpChild;
            AbstractNode* mpNext;
            AbstractNode** mpLastChildPtr;
            TokenIter mpFirst;
            TokenIter mpLast;
        };      

        // a typed node is a concrete realizateion of an AbstractNode
        // that is associated with a particular node
        template<typename Rule_T>
        struct TypedNode : AbstractNode
        {            
            // this is the type of the iterator 
            typedef typename AbstractNode::TokenIter TokenIter;

            // constructor
            TypedNode(TokenIter pos, AbstractNode* parent) 
                : AbstractNode(pos, parent)
            {  
            }            

            // returns the actual type of the rule
            virtual const std::type_info& GetRuleTypeInfo() {
                return typeid(Rule_T);
            }
        };


        template<typename Rule_T>
        static TypedNode<Rule_T>*
        NewChild(typename AbstractNode::TokenIter pos, AbstractNode* an)
        {
            //printf("%s\n", typeid(Rule_T).name());
            assert(!an->IsCompleted());
            TypedNode<Rule_T>* ret = new TypedNode<Rule_T>(pos, an);
            an->AddChild(ret);
            return ret;
        }


        // acccess the root AbstractNode
        AbstractNode* GetRoot() {
            return &root;
        } 
        
        // CreateNode is called when an attempt is made to match a 
        // Store production rule
        template<typename Rule_T, typename ParserState_T>
        void CreateNode(ParserState_T& p) { 
            assert(current != NULL);
            typename ParserState_T::Iterator pos = p.GetPos();
#ifdef OPENDDS_USE_NESTED_TEMPLATE_FWDECL
            current = current->template NewChild<Rule_T>(pos);
#else
            current = NewChild<Rule_T>(pos, current);
#endif
            assert(current != NULL);
        }

        // CreateNode is called when a Store production rule
        // is successfully matched 
        template<typename ParserState_T>
        void CompleteNode(ParserState_T& p) {
            assert(current != NULL);
            typename ParserState_T::Iterator pos = p.GetPos();
            current->Complete(pos);
            assert(current->IsCompleted());       
            current = current->GetParent();
            assert(current != NULL);
        }

        // AbandonNode is called when a Store<Rule_T> production rule
        // fails to match
        template<typename ParserState_T>
        void AbandonNode(ParserState_T&) {    
            assert(current != NULL);
            AbstractNode* tmp = current;
            assert(!tmp->IsCompleted());
            current = current->GetParent();
            assert(current != NULL);
            current->DeleteChild(tmp);
        }

        // deletes the current tree. 
        void Clear() {
            assert(current == &root);
            root.Clear();
        }

    private:
         
        // the current node of the tree, used because construction is in steps.
        AbstractNode* current; 

        // the root node of the tree 
        TypedNode<void> root;       
    };
}

#endif 