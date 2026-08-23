  
-- Slang building
rule("slang")
  set_extensions(".slang")
  on_buildcmd_file(function (target, batchcmds, sourcefile, opt)
      local outputfile = sourcefile .. ".spirv"
        
        batchcmds:show_progress(opt.progress, "${color.build.object}compiling.slang %s", sourcefile)
        batchcmds:mkdir(path.directory(outputfile))
        batchcmds:vrunv("slangc", {
            sourcefile,
            "-target", "spirv",
            "-o", outputfile
        })
        
        batchcmds:add_depfiles(sourcefile)
        batchcmds:set_depmtime(os.mtime(outputfile))
        batchcmds:set_depcache(target:dependfile(outputfile))
    end)

target("shaders")
  set_kind("object")

    add_rules("slang")

    -- Build every .slang shader in this directory.
    add_files("*.slang")
