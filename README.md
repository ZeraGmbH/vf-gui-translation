Jenkins periodically runs lupdate to refresh the translations of the gui
Translators then can update the translations and commit their changes
Bitbake then clones this repo, runs lrelease and packages the .qm files
