#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

// A sentential form is a sequence of symbols that can be derived from the
// start symbol in a finite number of steps.
struct sentential_form {
  std::shared_ptr<sentential_form> parent;
  size_t parent_symbol_id;
  size_t parent_alternative_id;
  std::vector<size_t> symbols;
  size_t depth;
};

// Print a sentential form.
void print_sentential_form(
  const std::vector<std::string> &symbols,
  const sentential_form &s
) {
  for (auto symbol : s.symbols) {
    std::cout << symbols[symbol] << " ";
  }
  std::cout << "\n";
}

// Print each sentential form in a derivation.
void print_derivation(
  const std::vector<std::string> &symbols,
  const sentential_form &s,
  size_t continuing_indentation
) {
  if (s.parent) {
    print_derivation(symbols, *(s.parent), continuing_indentation);
    for (size_t i = 0; i < continuing_indentation; ++i) {
      std::cout << " ";
    }
  }
  auto depth_str = std::to_string(s.depth);
  std::cout << depth_str << ": ";
  print_sentential_form(symbols, s);
}

// Determine whether there exists a sentence that can be derived
// from a given nonterminal symbol.
bool nonterminal_parsable(
  const std::vector<std::vector<std::vector<size_t>>> &rules,
  const std::unordered_set<size_t> &visited_symbols,
  size_t symbol
) {
  auto new_visited_symbols = visited_symbols;
  new_visited_symbols.insert(symbol);
  for (auto alternative : rules[symbol]) {
    bool alternative_parseable = true;
    for (auto alt_symbol : alternative) {
      if (visited_symbols.find(alt_symbol) != visited_symbols.end()) {
        alternative_parseable = false;
        break;
      }
      if (
        !rules[alt_symbol].empty() &&
        !nonterminal_parsable(rules, new_visited_symbols, alt_symbol)
      ) {
        alternative_parseable = false;
      }
    }
    if (alternative_parseable) {
      return true;
    }
  }
  return false;
}

// Determine if two sentential forms are equivalent up to the order in which
// rules are applied. This is a helper function for the `equivalent(...)`
// function below.
bool equivalent_helper(
  const std::vector<std::vector<std::vector<size_t>>> &rules,
  const std::vector<sentential_form const *> &frame_a,
  const std::vector<sentential_form const *> &frame_b,
  size_t frame_pos_a,
  size_t frame_pos_b,
  size_t symbol_pos_a,
  size_t symbol_pos_b
) {
  // Some helpful bindings
  auto sentential_form_a = frame_a[frame_pos_a];
  auto sentential_form_b = frame_b[frame_pos_b];
  auto sentential_form_a_child =
    (frame_pos_a > 0) ? frame_a[frame_pos_a - 1] : nullptr;
  auto sentential_form_b_child =
    (frame_pos_b > 0) ? frame_b[frame_pos_b - 1] : nullptr;
  auto symbol_a = sentential_form_a->symbols[symbol_pos_a];
  auto symbol_b = sentential_form_b->symbols[symbol_pos_b];
  auto symbol_pos_from_child_a =
    sentential_form_a_child ?
    sentential_form_a_child->parent_symbol_id :
    SIZE_MAX;
  auto symbol_pos_from_child_b =
    sentential_form_b_child ?
    sentential_form_b_child->parent_symbol_id :
    SIZE_MAX;
  auto alternative_from_child_a =
    sentential_form_a_child ?
    sentential_form_a_child->parent_alternative_id :
    SIZE_MAX;
  auto alternative_from_child_b =
    sentential_form_b_child ?
    sentential_form_b_child->parent_alternative_id :
    SIZE_MAX;

  // Synchronize sentential_form_a to then next applicable rule.
  if (frame_pos_a > 0) {
    if (symbol_pos_a != symbol_pos_from_child_a) {
      return equivalent_helper(
        rules,
        frame_a,
        frame_b,
        frame_pos_a - 1,
        frame_pos_b,
        symbol_pos_a + (
          (symbol_pos_a < symbol_pos_from_child_a) ?
          0 :
          (rules[sentential_form_a->symbols[symbol_pos_from_child_a]][
            alternative_from_child_a
          ].size() - 1)
        ),
        symbol_pos_b
      );
    }
  }

  // Synchronize sentential_form_b to then next applicable rule.
  if (frame_pos_b > 0) {
    if (symbol_pos_b != symbol_pos_from_child_b) {
      return equivalent_helper(
        rules,
        frame_a,
        frame_b,
        frame_pos_a,
        frame_pos_b - 1,
        symbol_pos_a,
        symbol_pos_b + (
          (symbol_pos_b < symbol_pos_from_child_b) ?
          0 :
          (rules[sentential_form_b->symbols[symbol_pos_from_child_b]][
            alternative_from_child_b
          ].size() - 1)
        )
      );
    }
  }

  // We better arrive at the bottom at the same time.
  if (frame_pos_a == 0 || frame_pos_b == 0) {
    return frame_pos_a == frame_pos_b;
  }

  // Make sure the two frames agree on the symbol and the rule.
  if (symbol_a != symbol_b) {
    return false;
  }
  if (alternative_from_child_a != alternative_from_child_b) {
    return false;
  }

  // For each symbol in the alternative, recurse on the child sentential form.
  for (
    size_t i = 0;
    i < rules[symbol_a][alternative_from_child_a].size();
    ++i
  ) {
    if (!equivalent_helper(
      rules,
      frame_a,
      frame_b,
      frame_pos_a - 1,
      frame_pos_b - 1,
      symbol_pos_a + i,
      symbol_pos_b + i
    )) {
      return false;
    }
  }

  // If we made it this far, all the descendants agree and the sentential forms
  // are equivalent.
  return true;
}

// For context-free grammars, the order in which rules are applied doesn't
// matter. This function determines if two derivations are equivalent, modulo
// rule application order. For example, the following two derivations are
// equivalent:
//
// Derivation 1:
//
//   0: expression
//   1: sum
//   2: expression + expression
//   3: expression + number
//   4: number + number
//
// Derivation 2:
//
//   0: expression
//   1: sum
//   2: expression + expression
//   3: number + expression
//   4: number + number
//
// If two derivations are equivalent according to this function, we don't count
// that as an ambiguity.
bool equivalent(
  const std::vector<std::vector<std::vector<size_t>>> &rules,
  const sentential_form &sentential_form_a,
  const sentential_form &sentential_form_b
) {
  // Build up a vector for each derivation by traversing the parent pointers.
  sentential_form const *current;
  std::vector<sentential_form const *> frame_a;
  std::vector<sentential_form const *> frame_b;
  current = &sentential_form_a;
  frame_a.push_back(current);
  while (current->parent) {
    current = current->parent.get();
    frame_a.push_back(current);
  }
  current = &sentential_form_b;
  frame_b.push_back(current);
  while (current->parent) {
    current = current->parent.get();
    frame_b.push_back(current);
  }

  // Let equivalent_helper(...) recursively check the whole tree from the root.
  return equivalent_helper(
    rules,
    frame_a,
    frame_b,
    frame_a.size() - 1,
    frame_b.size() - 1,
    0,
    0
  );
}

// Register a symbol if it was not already registered.
size_t register_symbol(
  std::vector<std::string> &symbols,
  std::vector<std::vector<std::vector<size_t>>> &rules,
  const std::string &symbol
) {
  for (size_t i = 0; i < symbols.size(); ++i) {
    if (symbol == symbols[i]) {
      return i;
    }
  }
  std::vector<std::vector<size_t>> rule;
  symbols.push_back(symbol);
  rules.push_back(rule);
  return symbols.size() - 1;
}

// Program entry point
int main(int argc, char *argv[]) {
  // Make sure we got a filename.
  if (argc != 2) {
    std::cout << "Usage: cfg-checker file.cfg\n";
    return 1;
  }

  // Read the file.
  std::ifstream file(argv[1]);
  if (!file.is_open()) {
    std::cout << "Unable to open file '" << argv[1] << "'.\n";
    return 1;
  }
  std::stringstream file_buffer;
  file_buffer << file.rdbuf();
  file.close();
  auto grammar = file_buffer.str();
  file.close();

  // Split the file into lines.
  std::vector<std::string> lines = { "" };
  for (size_t i = 0; i < grammar.size(); ++i) {
    if (grammar[i] == '\n') {
      lines.push_back("");
    } else {
      lines.back() += grammar[i];
    }
  }

  // An array of symbols in the grammar.
  // For performance, we generally we refer to a symbol by its ID into this
  // array rather than its name.
  std::vector<std::string> symbols;

  // Each rule is an array of alternatives, and
  // each alternative is an array of symbols.
  std::vector<std::vector<std::vector<size_t>>> rules;

  // Parse the grammar line by line.
  for (size_t i = 0; i < lines.size(); ++i) {
    // Split the line into tokens.
    auto line = lines[i];
    std::vector<std::string> tokens = { "" };
    for (size_t i = 0; i < line.size(); ++i) {
      if (
        line[i] == ' ' ||
        line[i] == '\t' ||
        line[i] == '\r' ||
        line[i] == '\n'
      ) {
        if (tokens.back() != "") {
          tokens.push_back("");
        }
      } else {
        tokens.back() += line[i];
      }
    }
    while (!tokens.empty() && tokens.back() == "") {
      tokens.pop_back();
    }

    // Make sure we have enough tokens to read.
    if (tokens.empty()) {
      continue;
    }
    if (tokens.size() < 2) {
      std::cout << "Bad production rule on line " << i + 1 << ".\n";
      return 1;
    }

    // Read the nonterminal on the left of the equals sign.
    auto nonterminal = tokens[0];
    if (!rules[register_symbol(symbols, rules, nonterminal)].empty()) {
      std::cout << "Multiple rules for nonterminal '" << nonterminal << "'.\n";
      return 1;
    }

    // Make sure the second token is an equals sign.
    if (tokens[1] != "=") {
      std::cout << "Bad production rule on line " << i + 1 << ".\n";
      return 1;
    }

    // Read the tokens on the right side of the equals sign.
    auto right_symbols = std::vector<std::string>(
      tokens.begin() + 2,
      tokens.end()
    );

    // Split the tokens into alternatives.
    std::vector<std::vector<std::string>> alternatives = {
      std::vector<std::string>()
    };
    for (size_t i = 0; i < right_symbols.size(); ++i) {
      if (right_symbols[i] == "|") {
        alternatives.push_back(std::vector<std::string>());
      } else {
        alternatives.back().push_back(right_symbols[i]);
      }
    }

    // Register the symbols from the alternatives.
    for (auto &alternative : alternatives) {
      for (auto &symbol : alternative) {
        register_symbol(symbols, rules, symbol);
      }
    }

    // Create a rule from the alternatives.
    for (auto &alternative : alternatives) {
      for (size_t i = 0; i < symbols.size(); ++i) {
        if (symbols[i] == nonterminal) {
          // Look up each symbol in the alternative by name.
          std::vector<size_t> alternative_symbols;
          for (auto symbol : alternative) {
            for (size_t j = 0; j < symbols.size(); ++j) {
              if (symbols[j] == symbol) {
                alternative_symbols.push_back(j);
                break;
              }
            }
          }

          // Add the alternative to the rule.
          rules[i].push_back(alternative_symbols);
          break;
        }
      }
    }
  }

  // Check if the language generated by the grammar is empty.
  if (
    rules.empty() ||
    !nonterminal_parsable(rules, std::unordered_set<size_t>(), 0)
  ) {
    std::cout << "The language generated by the grammar is empty.\n";
    return 1;
  }

  // Eliminate nonterminals from which no sentences can be derived.
  for (size_t i = 0; i < rules.size(); ++i) {
    if (
      !rules[i].empty() &&
      !nonterminal_parsable(rules, std::unordered_set<size_t>(), i)
    ) {
      // Remove any alternatives which used the nonterminal.
      for (auto &rule : rules) {
        for (size_t j = 0; j < rule.size(); ++j) {
          bool symbol_found = false;
          for (auto symbol : rule[j]) {
            if (symbol == i) {
              symbol_found = true;
              break;
            }
          }
          if (symbol_found) {
            rule.erase(rule.begin() + j);
            --j;
          }
        }
      }

      // Remove the nonterminal and its rule.
      symbols.erase(symbols.begin() + i);
      rules.erase(rules.begin() + i);
      --i;
    }
  }

  // This visited set allows us to find duplicate sentential forms.
  std::unordered_set<
    std::shared_ptr<sentential_form>,
    std::function<size_t(const std::shared_ptr<sentential_form> key)>,
    std::function<bool(
      const std::shared_ptr<sentential_form> a,
      const std::shared_ptr<sentential_form> b
    )>
  > visited(
    1000,
    [&](const std::shared_ptr<sentential_form> key) {
      size_t hash = 0;
      for (auto symbol : key->symbols) {
        hash ^= 0x9e3779b9 + (hash << 6) + (hash >> 2) + symbol;
      }
      return hash;
    },
    [&](
      const std::shared_ptr<sentential_form> a,
      const std::shared_ptr<sentential_form> b
    ) {
      return a->symbols == b->symbols;
    }
  );

  // Start the search with the start symbol.
  std::queue<std::shared_ptr<sentential_form>> sentential_forms;
  auto s = std::make_shared<sentential_form>();
  s->symbols = { 0 };
  s->depth = 0;
  sentential_forms.push(s);
  visited.insert(s);

  // Apply rules in a loop as long as we can.
  size_t search_depth = 0;
  while (!sentential_forms.empty()) {
    // Pop a sentential form from the queue.
    auto s = sentential_forms.front();
    sentential_forms.pop();
    if (s->symbols.empty()) {
      continue;
    }

    // Occasionally print some dots to entertain the user.
    if (s->depth + 1 > search_depth) {
      search_depth = s->depth + 1;
      std::cout << "." << std::flush;
    }

    // Iterate over the sentential form.
    for (size_t i = 0; i < s->symbols.size(); ++i) {
      // Iterate over all the alternatives for the current symbol.
      auto symbol = s->symbols[i];
      auto &rule = rules[symbol];
      for (size_t j = 0; j < rule.size(); ++j) {
        // Create a new sentential form with the rule applied.
        auto t = std::make_shared<sentential_form>();
        t->parent = s;
        t->parent_symbol_id = i;
        t->parent_alternative_id = j;
        t->symbols.insert(
          t->symbols.end(),
          s->symbols.begin(),
          s->symbols.begin() + i
        );
        t->symbols.insert(
          t->symbols.end(),
          rule[j].begin(),
          rule[j].end()
        );
        t->symbols.insert(
          t->symbols.end(),
          s->symbols.begin() + i + 1,
          s->symbols.end()
        );
        t->depth = s->depth + 1;

        // Try to add it to the queue and the visited set. If a non-equivalent
        // derivation was already there, the grammar is ambiguous.
        auto conflict = visited.find(t);
        if (conflict == visited.end()) {
          sentential_forms.push(t);
          visited.insert(t);
        } else {
          if (!equivalent(rules, *t, *(*conflict))) {
            std::cout << "\nFound a sentential form with two different " \
              "derivations:\n\n  ";
            print_sentential_form(symbols, *t);
            std::cout << "\nDerivation 1:\n\n  ";
            print_derivation(symbols, *t, 2);
            std::cout << "\nDerivation 2:\n\n  ";
            print_derivation(symbols, *(*conflict), 2);
            return 1;
          }
        }
      }
    }
  }

  // If we made it this far, there are only a finite number of derivations and
  // we checked them all.
  std::cout << "The grammar is unambiguous.\n";
  return 0;
}
