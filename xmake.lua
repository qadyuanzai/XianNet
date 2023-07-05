set_languages("c++20")
target("test")
  set_kind("binary")
  add_files("source/**.cc")
  add_includedirs("source")
  before_run(function (target)
    os.cp("$(curdir)/resource/config.toml", target:targetdir "/config.toml")
  end)
    