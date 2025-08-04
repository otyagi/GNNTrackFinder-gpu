set(QA_VERSION 40311c791b0129126d656e7c4a085609849b1fd1)

set(QA_SRC_URL "https://git.cbm.gsi.de/CbmSoft/cbmroot_qa.git")

download_project_if_needed(PROJECT         QA_source
                           GIT_REPOSITORY  ${QA_SRC_URL}
                           GIT_TAG         ${QA_VERSION}
                           SOURCE_DIR      ${CMAKE_SOURCE_DIR}/qa_data
                           TEST_FILE       README.md
                          )

