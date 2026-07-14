#include "soul/base/base_repository.h"

namespace sc {

Result<std::string> BaseRepository::findById(const std::string& id) {
    (void)id;
    return Result<std::string>(Error(ErrorCode::NotImplemented, "Not implemented"));
}

Result<std::vector<std::string>> BaseRepository::findAll() {
    return Result<std::vector<std::string>>(Error(ErrorCode::NotImplemented, "Not implemented"));
}

bool BaseRepository::save(const std::string& entity) {
    (void)entity;
    return false;
}

bool BaseRepository::remove(const std::string& id) {
    (void)id;
    return false;
}

}