# The following patterns are to be excluded from the documentation build
tests/*
build/*

# Do not build .md files

*.md
**/*.md

# We need to ensure these are not processed twice which breaks figure numbering. They are included via index.rst.
**/multi_rate_hifi_src.rst
**/fixed_ratio_src.rst
**/resource_usage_asrc.rst
**/resource_usage_ssrc.rst