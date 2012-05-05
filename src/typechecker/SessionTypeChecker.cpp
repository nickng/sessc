/**
 * \file
 * Session Type Checker component of Session C programming framework.
 * This is implemented as a plugin for the LLVM/clang compiler.
 *
 */

#include <cstdio>
#include <sstream>
#include <stack>
#include <map>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/DeclVisitor.h"
#include "clang/AST/StmtVisitor.h"

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Frontend/CompilerInstance.h"

#include "st_node.h"
#include "canonicalise.h"

extern "C" {
  extern int yyparse(st_tree *tree);
  extern FILE *yyin;
}


using namespace clang;

namespace {

  class SessionTypeCheckingConsumer : 
      public ASTConsumer,
      public DeclVisitor<SessionTypeCheckingConsumer>,
      public StmtVisitor<SessionTypeCheckingConsumer> {

    // AST-related fields.
    ASTContext    *context_;
    SourceManager *src_mgr_;
    TranslationUnitDecl *tu_decl_;

    // Local fields for building session type tree.
    st_tree *scribble_tree_;
    st_tree *tree_;
    std::stack< st_node * > appendto_node;
    std::map< std::string, std::string > varname2rolename;

    int caseState, ifState;
    int breakStmt_count, branch_count, outbranch_count, chain_count;
    int recur_counter;

    // Local variables for Visitors.
    Decl *current_decl_;
    typedef DeclVisitor<SessionTypeCheckingConsumer> BaseDeclVisitor;
    typedef StmtVisitor<SessionTypeCheckingConsumer> BaseStmtVisitor;

    public:

      virtual void Initialise(ASTContext &ctx) {
        context_ = &ctx;
        src_mgr_ = &context_->getSourceManager();
        tu_decl_ = context_->getTranslationUnitDecl();

        // Session Type tree root.
        tree_ = (st_tree *)malloc(sizeof(st_tree));

        chain_count = 0;
        caseState = 0;
        ifState = 0;
        breakStmt_count = 0;
        branch_count = 0;
        outbranch_count = 0;

        // Recursion label generation.
        recur_counter = 0;

        st_tree_init(tree_);
        st_tree_set_name(tree_, "_");

      } // Initialise()


      virtual void HandleTranslationUnit(ASTContext &ctx) {
        TranslationUnitDecl *tu_decl = context_->getTranslationUnitDecl();

        for (DeclContext::decl_iterator
             iter = tu_decl->decls_begin(), iter_end = tu_decl->decls_end();
             iter != iter_end; ++iter) {
          // Walk the AST to get source code ST tree.
          Decl *decl = *iter;
          BaseDeclVisitor::Visit(decl);
        }

        llvm::outs() << "\nScribble Protocol " << scribble_tree_->info->name << " at " <<  scribble_tree_->info->myrole << "\n";
        st_tree_print(scribble_tree_);

        llvm::outs() << "\nSource code Session Type\n";
        st_node_refactor(tree_->root);
        st_node_canonicalise(tree_->root);
        st_tree_print(tree_);

        st_tree_free(tree_);
        st_tree_free(scribble_tree_);
      }

      /* Auxiliary functions------------------------------------------------- */

      // Add to branch counter if it is inside the IF statement block (including THEN and ELSE)
      int addtoBranch_counter() {
        if (ifState == 1) {
          branch_count++;
          return 1;
        }
        return 0;
      }

      std::string get_rolename(Expr *expr) {
        std::string rolename("");

        if (isa<ImplicitCastExpr>(expr)) { // role*

          if (ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(expr)) {
            if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(ICE->getSubExpr())) {
              if (VarDecl *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
                // Map variable name to role name
                rolename = varname2rolename.at(VD->getNameAsString());
              }
            }
          }

        } else if (isa<CallExpr>(expr)) { // session->role(s, r);

          if (CallExpr *CE = dyn_cast<CallExpr>(expr)) {
            if (ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(CE->getCallee())) {

              // hack to check that this is a role-extraction function
              if (ICE->getType().getAsString().compare("role *(*)(struct session_t *, char *)") == 0) {

                // Now extract the second argument of ->role(s, r)
                if (ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(CE->getArg(1))) { 
                  if (StringLiteral *SL = dyn_cast<StringLiteral>(ICE->getSubExpr())) {
                    rolename = SL->getString();
                  } else {
                    llvm::errs() << "Invalid use of r(), expecting string literal";
                    rolename = std::string("__(variable)");
                  }
                }

              } // if correct type

            }
          }

        } else { // Not role* nor session->r(session, rolename)

          llvm::errs() << "Warning: unable to extract rolename (supported methods: role * declaration or s->r(s, name))\n";
          rolename = std::string("__(anonymous)");

        }

        //
        // Based on the extract rolename we infer the roles list of the session tree
        //
        if (rolename[0] == '_')
          return rolename;
        for (int role_idx=0; role_idx<tree_->info->nrole; ++role_idx) {
          if (strcmp(tree_->info->roles[role_idx], rolename.c_str()) == 0) {
            return rolename; // Role already registered in st_tree. Return early.
          }
        }

        st_tree_add_role(tree_, rolename.c_str());
        return rolename;

      }


      /* Visitors------------------------------------------------------------ */

      // Generic visitor.
      void Visit(Decl *decl) {
        if (!decl) return;

        Decl *prev_decl = current_decl_;
        current_decl_ = decl;
        BaseDeclVisitor::Visit(decl);
        current_decl_ = prev_decl;
      }

      // Declarator (variable | function | struct | typedef) visitor.
      void VisitDeclaratorDecl(DeclaratorDecl *decl_decl) {
        BaseDeclVisitor::VisitDeclaratorDecl(decl_decl);
      }

      // Declaration visitor.
      void VisitDecl(Decl *D) {
        if (isa<FunctionDecl>(D) || isa<ObjCMethodDecl>(D) || isa<BlockDecl>(D))
          return;

        // Generate context from declaration.
        if (DeclContext *DC = dyn_cast<DeclContext>(D))
          cast<SessionTypeCheckingConsumer>(this)->VisitDeclContext(DC);
      }

      // Declaration context visitor.
      void VisitDeclContext(DeclContext *DC) {
        for (DeclContext::decl_iterator
             iter = DC->decls_begin(), iter_end = DC->decls_end();
             iter != iter_end; ++iter) {
          Visit(*iter);
        }
      }

      // Variable visitor.
      void VisitDeclRefExpr(DeclRefExpr* expr) {
      }

      // Function (declaration and body) visitor.
      void VisitFunctionDecl(FunctionDecl *D) {
        BaseDeclVisitor::VisitFunctionDecl(D);
        if (D->isThisDeclarationADefinition()) {
            BaseStmtVisitor::Visit(D->getBody());
        }
      }

      // Generic code block (declaration and body) visitor.
      void VisitBlockDecl(BlockDecl *D) {
        BaseDeclVisitor::VisitBlockDecl(D);
        BaseStmtVisitor::Visit(D->getBody());
      }

      // Variable declaration visitor.
      void VisitVarDecl(VarDecl *D) {
        BaseDeclVisitor::VisitVarDecl(D);

        // If variable is a role, keep track of it.
        if (src_mgr_->isFromMainFile(D->getLocation())
            && D->getType().getAsString() == "role *") {

          if (D->hasInit()) {
            varname2rolename[D->getNameAsString()] = get_rolename(D->getInit());
          } else {
            llvm::errs()
              << "Warn: role* declaration not allowed without initialiser!"
              << "Declaration ignored\n";
          }
          return; // Skip over the initialiser Visitor.
        }

        // Initialiser.
        if (Expr *Init = D->getInit())
          BaseStmtVisitor::Visit(Init);
      }

      /* Statement Visitors-------------------------------------------------- */

      void VisitDeclStmt(DeclStmt *stmt) {
        for (DeclStmt::decl_iterator
             iter = stmt->decl_begin(), iter_end = stmt->decl_end();
             iter != iter_end; ++iter)
          Visit(*iter);
      }

      void VisitBlockExpr(BlockExpr *expr) {
        // The BlockDecl is also visited by 'VisitDeclContext()'.
        // No need to visit it twice.
      }

      // Statement visitor.
      void VisitStmt(Stmt *stmt) {

        // Break statement inside "switch-case".
        if (isa<BreakStmt>(stmt) && caseState == 1) {
            breakStmt_count++;
        }

        // Function call statement.
        if (isa<CallExpr>(stmt)) { // FunctionCall
          CallExpr *callExpr = cast<CallExpr>(stmt);

          if (callExpr->getDirectCallee() != 0) {

            //
            // Order of evaluating a function CALL is different from
            // the order in the AST
            // We want to make sure the arguments are evaluated first
            // in a function call, before evaluating the func call itself
            //
        
            Stmt *func_call_stmt = NULL;
            std::string func_name(callExpr->getDirectCallee()->getNameAsString());
            std::string datatype;
            std::string role;
            std::string label;

            for (Stmt::child_iterator
                iter = stmt->child_begin(), iter_end = stmt->child_end();
                iter != iter_end; ++iter) {

                // Skip first child (FunctionCall Stmt).
                if (func_call_stmt == NULL) {
                func_call_stmt = *iter;
                continue;
                }

                // Visit function arguments.
                if (*iter) BaseStmtVisitor::Visit(*iter);
            }


            // Visit FunctionCall Stmt. 
            BaseStmtVisitor::Visit(func_call_stmt);

            // ---------- Initialisation ----------
            if (func_name.find("session_init") != std::string::npos) {

              llvm::outs() << func_name << "\n";

              Expr *value = callExpr->getArg(3); // Scribble endpoint
              std::string scribble_filepath;

              if (ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(value)) {
                if (ImplicitCastExpr *ICE2 = dyn_cast<ImplicitCastExpr>(ICE->getSubExpr())) {
                  if (StringLiteral *SL = dyn_cast<StringLiteral>(ICE2->getSubExpr())) {
                    scribble_filepath = SL->getString();
                  }
                }
              }


              // Parse the Scribble file.
              yyin = fopen(scribble_filepath.c_str(), "r");
              scribble_tree_ = st_tree_init((st_tree *)malloc(sizeof(st_tree)));
              if (yyin == NULL) {
                llvm::errs() << "Unable to open Scribble file.\n";
              } else {
                yyparse(scribble_tree_);
              }
              fclose(yyin);

              if (scribble_tree_ == NULL) { // ie. parse failed
                llvm::errs() << "ERROR: Unable to parse Scribble file.\n";
                return;
              }

              tree_->root = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT);
              appendto_node.push(tree_->root);

              return;
            }

            if (func_name.find("session_end") != std::string::npos) {

              llvm::outs() << func_name << "\n";

              return;
            }

            //
            // Basic assumption
            // 1. Both sending and receiving uses prefix_type_name format
            // 2. Arguments positions of sending and receiving are same
            //    (at least for role argument)
            //

            // ---------- Send ----------
            if (func_name.find("send_") != std::string::npos) {

              llvm::outs() << func_name << "\n";

              addtoBranch_counter();

              // Extract the datatype (last segment of function name).
              datatype = func_name.substr(func_name.find("_") + 1, std::string::npos);

              // Extract the role (second argument).
              role = get_rolename(callExpr->getArg(1));
              Expr *labelExpr = callExpr->getArg(2);
              if (ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(labelExpr)) {
                if (isa<ParenExpr>(ICE->getSubExpr())) {
                  // NULL
                  if (ParenExpr *PE = dyn_cast<ParenExpr>(ICE->getSubExpr())) {
                    if (CStyleCastExpr *CCE = dyn_cast<CStyleCastExpr>(PE->getSubExpr())) {
                      if (IntegerLiteral *IL = dyn_cast<IntegerLiteral>(CCE->getSubExpr())) {
                        if (IL->getValue() == 0) {
                          label = "";
                        }
                      }
                    }
                  }
                } else if (isa<IntegerLiteral>(ICE->getSubExpr())) {
                  // 0
                   if (IntegerLiteral *IL = dyn_cast<IntegerLiteral>(ICE->getSubExpr())) {
                     if (IL->getValue() == 0) {
                       label = "";
                     }
                   }
                } else if (isa<ImplicitCastExpr>(ICE->getSubExpr())) {
                  // String label
                  if (ImplicitCastExpr *ICE2 = dyn_cast<ImplicitCastExpr>(ICE->getSubExpr())) {
                    if (StringLiteral *SL = dyn_cast<StringLiteral>(ICE2->getSubExpr())) {
                      label = SL->getString();
                    }
                  }
                }
              }

              st_node *node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_SEND);
              node->interaction->from = NULL;
              node->interaction->nto = 1;
              node->interaction->to = (char **)calloc(sizeof(char *), node->interaction->nto);
              node->interaction->to[0] = (char *)calloc(sizeof(char), role.size()+1);
              strcpy(node->interaction->to[0], role.c_str());
              if (label.compare("") == 0) {
                node->interaction->msgsig.op = NULL;
              } else {
                node->interaction->msgsig.op = (char *)calloc(sizeof(char), label.size()+1);
                strcpy(node->interaction->msgsig.op, label.c_str());
              }
              node->interaction->msgsig.payload = (char *)calloc(sizeof(char), datatype.size()+1);
              strcpy(node->interaction->msgsig.payload, datatype.c_str());

              // Put new ST node in position (ie. child of previous_node).
              st_node * previous_node = appendto_node.top();
              st_node_append(previous_node, node);
                      
              return; // End of ST_NODE_SEND construction.
            }
            // ---------- End of Send -----------
            
            // ---------- Receive/Recv ----------
            if (func_name.find("receive_") != std::string::npos  // Indirect recv
                || func_name.find("recv_") != std::string::npos) { // Direct recv

              llvm::outs() << func_name << "\n";

              addtoBranch_counter();

              // Extract the datatype (last segment of function name).
              datatype = func_name.substr(func_name.find("_") + 1,
                                          std::string::npos);

              // Extract the role (second argument).
              Expr *expr = callExpr->getArg(1);
              if (ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(expr)) {
                if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(ICE->getSubExpr())) {
                  if (VarDecl *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
                    role = VD->getNameAsString();
                  }
                }
              }

              st_node *node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_RECV);
              node->interaction->from = (char *)calloc(sizeof(char), role.size()+1);
              strcpy(node->interaction->from, role.c_str());
              node->interaction->nto = 0;
              node->interaction->to = NULL;
              node->interaction->msgsig.op = NULL;
              node->interaction->msgsig.payload = (char *)calloc(sizeof(char), datatype.size()+1);
              strcpy(node->interaction->msgsig.payload, datatype.c_str());

              // Put new ST node in position (ie. child of previous_node).
              st_node * previous_node = appendto_node.top();
              st_node_append(previous_node, node);

              return; // end of ST_NODE_RECV construction.
            }
            // ---------- End of Receive/Recv ----------

          } else {
            //
            // With the exception of role-extraction function, ignore all non-direct function calls.
            //
            if (ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(callExpr->getCallee())) {
              if (ICE->getType().getAsString().compare("role *(*)(struct session_t *, char *)") != 0) {
                llvm::errs() << "Warn: Skipping over a non-direct function call\n";
                callExpr->dump();
              }
            }

          } // if direct function call

        } // if isa<CallExpr>


        // While statement.
        if (isa<WhileStmt>(stmt)) {
          WhileStmt *whileStmt = cast<WhileStmt>(stmt);

          st_node *node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_RECUR);
          std::ostringstream ss;
          ss << "_L" << recur_counter++;
          std::string loopLabel = ss.str();
          node->recur->label = (char *)calloc(sizeof(char), loopLabel.size()+1);
          strcpy(node->recur->label, loopLabel.c_str());

          st_node *previous_node = appendto_node.top();
          st_node_append(previous_node, node);
          appendto_node.push(node);

          BaseStmtVisitor::Visit(whileStmt->getBody());

          appendto_node.pop();

          return;
        } // isa<WhileStmt>

        // For statememt.
        if (isa<ForStmt>(stmt) ) {
          ForStmt *forStmt = cast<ForStmt>(stmt);

          st_node *node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_RECUR);
          std::ostringstream ss;
          ss << "_L" << recur_counter++;
          std::string loopLabel = ss.str();
          node->recur->label = (char *)calloc(sizeof(char), loopLabel.size()+1);
          strcpy(node->recur->label, loopLabel.c_str());

          st_node *previous_node = appendto_node.top();
          st_node_append(previous_node, node);
          appendto_node.push(node);

          BaseStmtVisitor::Visit(forStmt->getBody());

          appendto_node.pop();

          return;
        } // isa<ForStmt>

        // Do statement.
        if (isa<DoStmt>(stmt)) {
          DoStmt *doStmt = cast<DoStmt>(stmt);

          st_node *node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_RECUR);
          std::ostringstream ss;
          ss << "_L" << recur_counter++;
          std::string loopLabel = ss.str();
          node->recur->label = (char *)calloc(sizeof(char), loopLabel.size()+1);
          strcpy(node->recur->label, loopLabel.c_str());

          st_node *previous_node = appendto_node.top();
          st_node_append(previous_node, node);
          appendto_node.push(node);

          BaseStmtVisitor::Visit(doStmt->getBody());

          appendto_node.pop();

          return;
        } // isa<DoStmte>


        // Continue (within while-loop).
        if (isa<ContinueStmt>(stmt)) {
          st_node *previous_node = appendto_node.top();
          std::stack< st_node * > node_parents(appendto_node);

          while (!node_parents.empty()) {
            st_node *prev = node_parents.top();
            if (prev->type == ST_NODE_RECUR) {
              st_node *node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_CONTINUE);
              node->cont->label = (char *)calloc(sizeof(char), strlen(prev->recur->label)+1);
              strcpy(node->cont->label, prev->recur->label);
              st_node_append(previous_node, node);
              return;
            }
            node_parents.pop(); // Go up one level.
          }

          return;
        } // isa<ContinueStmt>


        // If statement.
        if (isa<IfStmt>(stmt)) {
          IfStmt *ifStmt = cast<IfStmt>(stmt);

          st_node *node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_CHOICE);

          st_node *previous_node = appendto_node.top();
          st_node_append(previous_node, node);
          appendto_node.push(node);

          // Then-block.
          if (ifStmt->getThen() != NULL) {
            st_node *then_node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT);
            st_node_append(node, then_node);
            appendto_node.push(then_node);
            BaseStmtVisitor::Visit(ifStmt->getThen());
            appendto_node.pop();
          }

          // Else-block.
          if (ifStmt->getElse() != NULL) {
            st_node *else_node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT);
            st_node_append(node, else_node);
            appendto_node.push(else_node);
            BaseStmtVisitor::Visit(ifStmt->getElse());
            appendto_node.pop();
          }

          appendto_node.pop();

          node->choice->at = NULL;
          for (int i=0; i<node->nchild; ++i) { // Children of choice = code blocks
            for (int j=0; j<node->children[i]->nchild; ++i) { // Children of code blocks = body of then/else
              if (node->children[i]->children[j]->type == ST_NODE_RECV) {
                node->choice->at = (char *)calloc(sizeof(char), strlen(node->children[0]->children[i]->interaction->from)+1);
                strcpy(node->choice->at, node->children[0]->children[i]->interaction->from);
                break;
              }
              if (node->children[i]->children[j]->type == ST_NODE_SEND) {
                node->choice->at = (char *)calloc(sizeof(char), strlen(tree_->info->myrole)+1);
                strcpy(node->choice->at, tree_->info->myrole);
                break;
              }
            }
            if (node->choice->at != NULL) {
              // Found choice role.
              break;
            }
          }

          return;
        }


        // Child nodes.

        for (Stmt::child_iterator
             iter = stmt->child_begin(), iter_end = stmt->child_end();
             iter != iter_end; ++iter) {
          if (*iter) {
            BaseStmtVisitor::Visit(*iter);
          }
        }

      } // VisitStmt

    // public

  }; // class SessionTypeCheckingConsumer

/* -------------------------------------------------------------------------- */

  /**
   * Toplevel Plugin interface to dispatch type checker.
   *
   */
  class SessionTypeCheckingAction : public PluginASTAction {

    protected:

      ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) {
        SessionTypeCheckingConsumer *checker = new SessionTypeCheckingConsumer();
        if (CI.hasASTContext()) {
          checker->Initialise(CI.getASTContext());
        }

        return checker;
      }

      bool ParseArgs(const CompilerInstance &CI,
                     const std::vector<std::string>& args) {
        return true;
      }

  }; // class SessionTypeCheckingAction

} // (null) namespace

static FrontendPluginRegistry::Add<SessionTypeCheckingAction>
X("sess-type-check", "Session Type checker");
