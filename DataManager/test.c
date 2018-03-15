///*
// * test.c
// *
// *  Created on: Jan 20, 2017
// *      Author: Jeff
// */
//
//ExpMatch helper(Parser *parser) {
//  return ( {ExpMatch __fn__(Parser *parser) {
//          return match_merge(parser, ( {ExpMatch __fn__(Parser *parser) {
//                      Token *tok = parser_next_skip_ln__(parser, ((void *)0));
//                      if (((void *)0) == tok || LPAREN != tok->type) return no_match(parser);
//                      return match(parser);
//                    }__fn__;
//                  })(parser), ( {ExpMatch __fn__(Parser *parser) {
//                      return match_merge(parser, primary_expression(parser), ( {ExpMatch __fn__(Parser *parser) {
//                                  Token *tok = parser_next_skip_ln__(parser, ((void *)0));
//                                  if (((void *)0) == tok || RPAREN != tok->type) return no_match(parser);
//                                  return match(parser);
//                                }__fn__;
//                              })(parser));
//                    }__fn__;
//                  })(parser));
//        }__fn__;
//      })(parser);
//}
//
//ExpMatch helper(Parser *parser) {
//  return ( {ExpMatch __fn__(Parser *parser) {
//          ExpMatch m1 = ( {ExpMatch __fn__(Parser *parser) {
//                  Token *tok = parser_next_skip_ln__(parser, ((void *)0));
//                  if (((void *)0) == tok || LPAREN != tok->type) {
//                    return no_match(parser);
//                  }
//                  return match(parser);
//                }__fn__;
//              })(parser);
//          if (m1.matched) return m1;
//          ExpMatch m2 = ( {ExpMatch __fn__(Parser *parser) {
//                  ExpMatch m1 = primary_expression(parser);
//                  if (m1.matched) return m1;
//                  ExpMatch m2 = ( {ExpMatch __fn__(Parser *parser) {
//                          Token *tok = parser_next_skip_ln__(parser, ((void *)0));
//                          if (((void *)0) == tok || RPAREN != tok->type) {
//                            return no_match(parser);
//                          }
//                          return match(parser);
//                        }__fn__;
//                      })(parser);
//                  if (m2.matched) return m2;
//                  return no_match(parser);
//                }__fn__;
//              })(parser);
//          if (m2.matched) return m2;
//          return no_match(parser);
//        }__fn__;
//      })(parser);
//}
