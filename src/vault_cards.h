#ifndef VAULT_CARDS_H
#define VAULT_CARDS_H

namespace dhf::vault_cards {

/// True if vault cards should be allowed to generate challenges. Defaults to false.
extern bool enable;

/**
 * @brief Initalizes vault card suppression.
 */
void init(void);

}  // namespace dhf::vault_cards

#endif /* VAULT_CARDS_H */
