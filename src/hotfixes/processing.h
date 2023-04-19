#ifndef HOTFIXES_PROCESSING_H
#define HOTFIXES_PROCESSING_H

namespace dhf::hotfixes {

struct FJsonObject;

/**
 * @brief Handles `GbxSparkSdk::Discovery::Services::FromJson` calls, inserting our custom hotfixes.
 *
 * @param json Unreal json objects containing the received data.
 */
void handle_discovery_from_json(FJsonObject** json);

/**
 * @brief Handles `GbxSparkSdk::News::NewsResponse::FromJson` calls, inserting our custom article.
 *
 * @param json Unreal json objects containing the received data.
 */
void handle_news_from_json(FJsonObject** json);

}  // namespace dhf::hotfixes

#endif /* HOTFIXES_PROCESSING_H */
