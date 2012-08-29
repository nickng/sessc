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

    protected:

    typedef DeclVisitor<SessionTypeCheckingConsumer> BaseDeclVisitor;
    typedef StmtVisitor<SessionTypeCheckingConsumer> BaseStmtVisitor;

    // AST-related fields.
    ASTContext    *context_;
    SourceManager *src_mgr_;
    TranslationUnitDecl *tu_decl_;
    Decl          *current_decl_;

    // Local fields for building session type tree.
    st_tree *scribble_tree_;
    st_tree *tree_;
    std::stack< st_node * > appendto_node;
    std::map< std::string, std::string > varname2rolename;

    std::stack< st_expr_t * > cond_stack;
    std::stack< st_expr_t * > role_cond_stack;

    // Recursion counter.
    int recur_counter;

    int caseState, ifState;
    int breakStmt_count, branch_count, outbranch_count, chain_count;

    public:

      virtual void Initialise(ASTContext &ctx) {
        context_ = &ctx;
        src_mgr_ = &context_->getSourceManager();
        tu_decl_ = context_->getTranslationUnitDecl();

        // Session Type tree root.
        tree_ = (st_tree *)malloc(sizeof(st_tree));

        // Recursion label generation.
        recur_counter = 0;

        st_tree_init(tree_);
        st_tree_set_name(tree_, "_");
        tree_->info->myrole = strdup("__ROLE__");

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

        // Scribble protocol.
        st_node_canonicalise(scribble_tree_->root);

        // Source code.
        st_node_refactor(tree_->root);
        st_node_canonicalise(tree_->root);

        unsigned diagId;
        // Do type checking (comparison)
        st_node_reset_markedflag(tree_->root);
        st_node_reset_markedflag(scribble_tree_->root);
        if (st_node_compare_r(scribble_tree_->root, tree_->root)) {
          diagId = context_->getDiagnostics().getCustomDiagID(DiagnosticsEngine::Note, "Type checking successful");
          st_tree_print(tree_);
          st_tree_print(scribble_tree_);
          llvm::outs() << "Type checking successful\n";
        } else {
          diagId = context_->getDiagnostics().getCustomDiagID(DiagnosticsEngine::Error, "Type checking failed, see above for error location");

          // Show the error
          llvm::errs() << "********** Scribble tree: **********";
          st_tree_print(scribble_tree_);
          llvm::errs() << "********** Source code tree: **********";
          st_tree_print(tree_);
        }

        SourceLocation SL = tu_decl->getLocation();
        context_->getDiagnostics().Report(SL, diagId);


        st_tree_free(tree_);
        st_tree_free(scribble_tree_);
      }

      /* Auxiliary functions------------------------------------------------- */

      std::string get_rolename(Expr *expr) {
        std::string rolename("");

        if (isa<ImplicitCastExpr>(expr)) { // role*
          ImplicitCastExpr *ICE = cast<ImplicitCastExpr>(expr);
          if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(ICE->getSubExpr())) {
            if (VarDecl *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
              // Map variable name to role name
              rolename = varname2rolename.at(VD->getNameAsString());
            }
          }

        } else if (isa<CallExpr>(expr)) { // session->role(s, ?);
          CallExpr *CE = cast<CallExpr>(expr);

          if (ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(CE->getCallee())) {

            // Is this s->r(s, rolename)
            if (ICE->getType().getAsString().compare("role *(*)(struct session_t *, char *)") == 0) {

              // Now extract the second argument of ->role(s, r)
              if (ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(CE->getArg(1))) {
                if (StringLiteral *SL = dyn_cast<StringLiteral>(ICE->getSubExpr())) {
                  rolename = SL->getString();
                } else {
                  int diagId = context_->getDiagnostics().getCustomDiagID(DiagnosticsEngine::Error, "Invalid use of r(), expecting string literal");
                  context_->getDiagnostics().Report(diagId) << SL->getSourceRange();
                  rolename = std::string("__(variable)");
                }
              }

            // Is this s->i(s, index)
            } else if (ICE->getType().getAsString().compare("role *(*)(struct session_t *, int)") == 0) {

              // Extract second argument of ->role(s, r)
              st_expr_print(parseExpr(CE->getArg(1)));
              rolename = std::string(scribble_tree_->info->myrole);

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
          if (strcmp(tree_->info->roles[role_idx]->name, rolename.c_str()) == 0) {
            return rolename; // Role already registered in st_tree. Return early.
          }
        }

        st_tree_add_role(tree_, rolename.c_str());
        return rolename;

      }

      st_expr_t *parseExpr(Expr *E) {

        //
        // Basic binary operators
        //
        // covers all st_expr supported binary operators
        // except toplevel expressions: RANGE and TUPLE
        //
        if (isa<BinaryOperator>(E)) {
          BinaryOperator *BO = cast<BinaryOperator>(E);
          switch (BO->getOpcode()) {
            case BO_Add:
              return st_expr_binexpr(parseExpr(BO->getLHS()), ST_EXPR_TYPE_PLUS, parseExpr(BO->getRHS()));
              break;
            case BO_Sub:
              return st_expr_binexpr(parseExpr(BO->getLHS()), ST_EXPR_TYPE_MINUS, parseExpr(BO->getRHS()));
              break;
            case BO_Mul:
              return st_expr_binexpr(parseExpr(BO->getLHS()), ST_EXPR_TYPE_MULTIPLY, parseExpr(BO->getRHS()));
              break;
            case BO_Div:
              return st_expr_binexpr(parseExpr(BO->getLHS()), ST_EXPR_TYPE_DIVIDE, parseExpr(BO->getRHS()));
              break;
            case BO_Rem:
              return st_expr_binexpr(parseExpr(BO->getLHS()), ST_EXPR_TYPE_MODULO, parseExpr(BO->getRHS()));
              break;
            case BO_Shl:
              return st_expr_binexpr(parseExpr(BO->getLHS()), ST_EXPR_TYPE_SHL, parseExpr(BO->getRHS()));
              break;
            case BO_Shr:
              return st_expr_binexpr(parseExpr(BO->getLHS()), ST_EXPR_TYPE_SHR, parseExpr(BO->getRHS()));
              break;
            case BO_EQ:
              return st_expr_binexpr(parseExpr(BO->getLHS()), ST_EXPR_TYPE_EQUAL, parseExpr(BO->getRHS()));
              break;
            default:
              llvm::errs() << "Warning: Unknown Opcode " << BO->getOpcode() << ", returning 1 (true)\n";
              return st_expr_constant(1);
          }
        }

        //
        // Basic unary operators
        //
        if (isa<UnaryOperator>(E)) {
          UnaryOperator *UO = cast<UnaryOperator>(E);
          switch (UO->getOpcode()) {
            case UO_Minus:
              return st_expr_binexpr(st_expr_constant(0), ST_EXPR_TYPE_MINUS, parseExpr(UO->getSubExpr()));
              break;
            default:
              llvm::errs() << "Warning: Unknown Opcode " << UO->getOpcode() << ", returning 1 (true)\n";
              return st_expr_constant(1);
          }
        }

        //
        // Integer literals
        //
        if (isa<IntegerLiteral>(E)) {
          IntegerLiteral *IL = cast<IntegerLiteral>(E);
          int il_val = *(IL->getValue().getRawData());
          return st_expr_constant(il_val);
        }

        //
        // Parenthesis (ignore)
        //
        if (isa<ParenExpr>(E)) {
          ParenExpr *PE = cast<ParenExpr>(E);
          return parseExpr(PE->getSubExpr());
        }

        //
        // Multiple possibilities
        //
        // 1. Plain ariable (DeclRefExpr) (int *)i
        // 2. Struct member (MemberExpr,ImplicitCastExpr,DeclRefExpr) (int)->member((session *)s)
        //
        if (isa<ImplicitCastExpr>(E)) {
          ImplicitCastExpr *ICE = cast<ImplicitCastExpr>(E);
          if (isa<DeclRefExpr>(ICE->getSubExpr())) {
            DeclRefExpr *DRE = cast<DeclRefExpr>(ICE->getSubExpr());
            return st_expr_variable(strdup(DRE->getDecl()->getNameAsString().c_str()));
          }
          if (isa<MemberExpr>(ICE->getSubExpr())) {
            MemberExpr *ME = cast<MemberExpr>(ICE->getSubExpr());
            if (ME->getMemberDecl()->getNameAsString().compare("index") == 0
                && ME->getBase()->getType().getAsString().compare("session *") == 0) {
              return st_expr_variable(strdup("__INDEX__"));
            }
          }
        }

        llvm::outs() << "Cannot convert [start]\n";
        E->dump();
        llvm::outs() << "Cannot convert [end]\n";
        return st_expr_constant(1);
      }

      st_expr_t **getRange(InitListExpr *rangeExpr) {
        st_expr_t **rng = (st_expr_t **)calloc(sizeof(st_expr_t *), rangeExpr->getNumInits()/2);
        //rng[i] = st_expr_binexpr(parseExpr(rangeExpr->getChild(2*i)), ST_EXPR_TYPE_RANGE, parseExpr(rangeExpr->getChild(2*i+1)));

        return rng;
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

              node->interaction->to = (st_role_t **)calloc(sizeof(st_role_t *), node->interaction->nto);
              node->interaction->to[0] = (st_role_t *)malloc(sizeof(st_role_t));
              node->interaction->to[0]->name = strdup(role.c_str());
              if (label.compare("") == 0) {
                node->interaction->msgsig.op = NULL;
              } else {
                node->interaction->msgsig.op = (char *)calloc(sizeof(char), label.size()+1);
                strcpy(node->interaction->msgsig.op, label.c_str());
              }
              node->interaction->msgsig.payload = (char *)calloc(sizeof(char), datatype.size()+1);
              strcpy(node->interaction->msgsig.payload, datatype.c_str());

              // Evaluation conditions
              node->interaction->cond = cond_stack.top();
              if (NULL != role_cond_stack.top()) { // If role condition is not NULL
                node->interaction->msg_cond = (msg_cond_t *)malloc(sizeof(msg_cond_t));
                node->interaction->msg_cond->name = tree_->info->myrole;
                node->interaction->msg_cond->param = role_cond_stack.top();
              }

              // Put new ST node in position (ie. child of previous_node).
              st_node * previous_node = appendto_node.top();
              st_node_append(previous_node, node);

              return; // End of ST_NODE_SEND construction.
            }
            // ---------- End of Send -----------

            // ---------- Receive/Recv ----------
            if (func_name.find("receive_") != std::string::npos  // Indirect recv
                || func_name.find("recv_") != std::string::npos) { // Direct recv

              // Extract the datatype (last segment of function name).
              datatype = func_name.substr(func_name.find("_") + 1,
                                          std::string::npos);

              // Extract the role (second argument).
              role = get_rolename(callExpr->getArg(1));

              st_node *node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_RECV);
              node->interaction->from = (st_role_t *)malloc(sizeof(st_role_t));
              node->interaction->from->name = strdup(role.c_str());
              node->interaction->nto = 0;
              node->interaction->to = NULL;
              node->interaction->msgsig.op = NULL;
              node->interaction->msgsig.payload = (char *)calloc(sizeof(char), datatype.size()+1);
              strcpy(node->interaction->msgsig.payload, datatype.c_str());
              node->interaction->cond = NULL;
              node->interaction->msg_cond = NULL;

              // Evaluation conditions
              node->interaction->cond = cond_stack.top();
              if (NULL != role_cond_stack.top()) { // If role condition is not NULL
                node->interaction->msg_cond = (msg_cond_t *)malloc(sizeof(msg_cond_t));
                node->interaction->msg_cond->name = tree_->info->myrole;
                node->interaction->msg_cond->param = role_cond_stack.top();
              }

              // Put new ST node in position (ie. child of previous_node).
              st_node * previous_node = appendto_node.top();
              st_node_append(previous_node, node);

              return; // end of ST_NODE_RECV construction.
            }
            // ---------- End of Receive/Recv ----------

            // ---------- Receive label ----------
            if (func_name.compare("probe_label") == 0) {

              std::string payload("__LABEL__");

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
              node->interaction->from = (st_role_t *)malloc(sizeof(st_role_t));
              node->interaction->from->name = strdup(role.c_str());
              node->interaction->nto = 0;
              node->interaction->to = NULL;
              node->interaction->msgsig.op = NULL;
              node->interaction->msgsig.payload = (char *)calloc(sizeof(char), payload.size()+1);
              strcpy(node->interaction->msgsig.payload, payload.c_str());

              // Put new ST node in position (ie. child of previous_node).
              st_node * previous_node = appendto_node.top();
              st_node_append(previous_node, node);

              return; // end of ST_NODE_RECV construction.
            }
            // ---------- End of Receive label ----------

          } else {
            //
            // With the exception of role-extraction function, ignore all non-direct function calls.
            //
            if (ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(callExpr->getCallee())) {
              if (ICE->getType().getAsString().compare("role *(*)(struct session_t *, char *)") != 0
                  && ICE->getType().getAsString().compare("role *(*)(struct session_t *, int)") != 0) {
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

          // Implicit continue at end of loop.
          st_node *node_end = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_CONTINUE);
          node_end->cont->label = (char *)calloc(sizeof(char), loopLabel.size()+1);
          strcpy(node_end->cont->label, loopLabel.c_str());
          st_node_append(node, node_end);

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

          // Implicit continue at end of loop.
          st_node *node_end = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_CONTINUE);
          node_end->cont->label = (char *)calloc(sizeof(char), loopLabel.size()+1);
          strcpy(node_end->cont->label, loopLabel.c_str());
          st_node_append(previous_node, node_end);

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

          // Implicit continue at end of loop.
          st_node *node_end = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_CONTINUE);
          node_end->cont->label = (char *)calloc(sizeof(char), loopLabel.size()+1);
          strcpy(node_end->cont->label, loopLabel.c_str());
          st_node_append(previous_node, node_end);

          appendto_node.pop();

          return;
        } // isa<DoStmt>


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

          //
          // For an If-Statement, we try to match conditional evaluation in Scribble
          // by putting into stack cond_stack and role_cond_stack
          //
          st_expr_t *bool_cond = NULL; // Pure boolean condition
          st_expr_t *role_cond = NULL; // Role condition for pattern matching

          // If this is a boolean condition and with CallExpr && CallExpr
          if (isa<BinaryOperator>(ifStmt->getCond())) {

            BinaryOperator *BO = cast<BinaryOperator>(ifStmt->getCond());
            if (BO->getOpcode() == BO_LAnd
                && isa<CallExpr>(BO->getLHS()) && isa<CallExpr>(BO->getRHS())) {

              CallExpr *CE_LHS = cast<CallExpr>(BO->getLHS());
              CallExpr *CE_RHS = cast<CallExpr>(BO->getRHS());

              // We want to match sc_eval_cond(...) && sc_index_match(...)
              if (0 == CE_LHS->getDirectCallee()->getNameAsString().compare("sc_eval_cond")
                  && 0 == CE_RHS->getDirectCallee()->getNameAsString().compare("sc_index_match")) {

                if (isa<ImplicitCastExpr>(CE_RHS->getArg(1))) {
                  ImplicitCastExpr *ICE = cast<ImplicitCastExpr>(CE_RHS->getArg(1));
                  if (isa<CompoundLiteralExpr>(ICE->getSubExpr())) {
                    CompoundLiteralExpr *CLE = cast<CompoundLiteralExpr>(ICE->getSubExpr());
                    if (isa<InitListExpr>(CLE->getInitializer())) {
                      InitListExpr *ILE = cast<InitListExpr>(CLE->getInitializer());
                      for (unsigned i=0; i<ILE->getNumInits()/2; ++i) {
                        bool_cond = parseExpr(CE_LHS->getArg(0));
                        role_cond = st_expr_binexpr(parseExpr(ILE->getInits()[2*i]), ST_EXPR_TYPE_RANGE, parseExpr(ILE->getInits()[2*i+1]));
                      }
                    }
                  }
                }
              } else {
                int diagId = context_->getDiagnostics().getCustomDiagID(DiagnosticsEngine::Warning, "Expecting sc_eval_cond() and sc_index_match() for conditional evaluation");
                context_->getDiagnostics().Report(ifStmt->getCond()->getExprLoc(), diagId) << BO->getSourceRange();
              }

            }

          } else { // condition is not a BinaryOperator

            if (isa<CallExpr>(ifStmt->getCond())) {

              CallExpr *CE = cast<CallExpr>(ifStmt->getCond());

              // We want to match sc_index_match(...) only
              if (CE->getDirectCallee()->getNameAsString().compare("sc_index_match") == 0) {
                role_cond = st_expr_binexpr(parseExpr(CE->getArg(1)), ST_EXPR_TYPE_RANGE, parseExpr(CE->getArg(2)));
              }
            }

          }

          //
          // Push the conditions in the condition stacks for Then-block
          //
          cond_stack.push(bool_cond);
          role_cond_stack.push(role_cond);

          // Now create the choice node
          st_node *node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_CHOICE);

          st_node *previous_node = appendto_node.top();
          st_node_append(previous_node, node);
          appendto_node.push(node);

          // Then-block.
          if (ifStmt->getThen() != NULL) {
            st_node *then_node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_ROOT);
            st_node_append(node, then_node);
            appendto_node.push(then_node);

            if (isa<CallExpr>(ifStmt->getCond())) {
              if (CallExpr *CE = dyn_cast<CallExpr>(ifStmt->getCond())) {
                if (CE->getDirectCallee()->getNameAsString().compare("has_label") == 0) {
                  std::string payload("__LABEL__");
                  std::string op;
                  std::string role = "__LOCAL__";

                  // Extract the label (second argument).
                  if (ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(CE->getArg(1))) {
                    if (ImplicitCastExpr *ICE2 = dyn_cast<ImplicitCastExpr>(ICE->getSubExpr())) {
                      if (StringLiteral *SL = dyn_cast<StringLiteral>(ICE2->getSubExpr())) {
                        op = SL->getString();
                      }
                    }
                  }

                  // Append a dummy recv node
                  st_node *label_node = st_node_init((st_node *)malloc(sizeof(st_node)), ST_NODE_RECV);
                  label_node->interaction->from = (st_role_t *)malloc(sizeof(st_role_t));
                  label_node->interaction->from->name = strdup(role.c_str());
                  label_node->interaction->nto = 0;
                  label_node->interaction->to = NULL;
                  label_node->interaction->msgsig.op = (char *)calloc(sizeof(char), op.size()+1);
                  strcpy(label_node->interaction->msgsig.op, op.c_str());
                  label_node->interaction->msgsig.payload = (char *)calloc(sizeof(char), payload.size()+1);
                  strcpy(label_node->interaction->msgsig.payload, payload.c_str());

                  // Put new ST node in position (ie. child of previous_node).
                  st_node_append(then_node, label_node);

                }
              }
            }

            BaseStmtVisitor::Visit(ifStmt->getThen());
            appendto_node.pop();

          }

          // Restore the conditions
          cond_stack.pop();
          role_cond_stack.pop();

          if ((NULL != bool_cond || NULL != role_cond) && NULL != ifStmt->getElse()) {
            int diagId = context_->getDiagnostics().getCustomDiagID(DiagnosticsEngine::Warning, "Conditional evaluation condition set, Expecting empty Else-block but Else-block non-empty");
            context_->getDiagnostics().Report(diagId) << ifStmt->getElse()->getSourceRange();
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
            for (int j=0; j<node->children[i]->nchild; ++j) { // Children of code blocks = body of then/else
              if (node->children[i]->children[j]->type == ST_NODE_RECV) {
                if (strcmp(node->children[i]->children[j]->interaction->from->name, "__LOCAL__") == 0) {
                  continue;
                }

                node->choice->at = strdup(node->children[i]->children[j]->interaction->from->name);
                break;
              }
              if (node->children[i]->children[j]->type == ST_NODE_SEND) {
                node->choice->at = strdup(tree_->info->myrole);
                break;
              }
            }

            // If choice at role is found in one of the code blocks, stop search
            if (node->choice->at != NULL) {
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
