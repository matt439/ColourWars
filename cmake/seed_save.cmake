# Seeds the default save directory beside the exe, only when absent —
# a rebuild must never overwrite a real save file.
# Inputs: src (the repo's save directory), dst_parent (the exe directory).
if(NOT EXISTS "${dst_parent}/save")
    file(COPY "${src}" DESTINATION "${dst_parent}")
endif()
