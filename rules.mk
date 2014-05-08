#
# rules.mk: build rules
#
#  For automatic dependency resolution, set $(objects) to the list of object
#  files needed for the build before including this file.
#
#  $(clean) and $(distclean) should be set to the files to be removed on
#  'make clean' and 'make distclean', respectively.
#
#  $(verbose) controls whether build commands are echoed verbatim, or in
#  prettier "  PROG    FILE" format.  Set verbose=y for verbatim output.
#

.SUFFIXES:
.PHONY: clean distclean

dependencies = $(addprefix ., $(addsuffix .d, $(basename $(objects))))
distclean += $(dependencies)

ifeq ($(verbose),y)
  quiet =
else
  quiet = quiet_
endif

clean:
	$(if $(clean), rm -f $(clean))

distclean: clean
	$(if $(distclean), rm -f $(distclean))

%.o: %.c
	$(call cmd,cc)

.%.d: %.c
	$(call cmd,dep)

# CC for program object files (.o)
quiet_cmd_cc    = CC      $@
      cmd_cc    = $(CC) -c $(CPPFLAGS) $(ALLCFLAGS) -o $@ $<

# AS for program object files (.o)
quiet_cmd_as    = AS      $@
      cmd_as    = $(AS) $(ASFLAGS) -o $@ $<

# create archive
quiet_cmd_ar    = AR      $@
      cmd_ar    = $(AR) $(ARFLAGS) $@ $^

# LD for programs; optional parameter: libraries
quiet_cmd_ld    = LD      $@
      cmd_ld    = $(LD) $(LDFLAGS) -o $@ $^ $(1)

# scripted LD
quiet_cmd_sld   = LD      $@
      cmd_sld   = $(LD) -T $(1) $(LDFLAGS) -o $@

# LD for user programs
quiet_cmd_uld = ULD     $@
      cmd_uld = $(LD) -r $^ -o $@ && \
		$(OBJCOPY) -G main $@ && \
		$(OBJCOPY) --redefine-sym main=$(1) $@

# generate dependencies file
quiet_cmd_dep   = DEP     $@
      cmd_dep   = echo "$@ `$(CC) $(ALLCFLAGS) -MM -I $(incdir) $(CPPFLAGS) $<`" > $@

# call submake
quiet_cmd_smake = MAKE    $@
      cmd_smake = cd $@ && $(MAKE) $(1)

# concatenate prerequisites
quiet_cmd_cat    = GEN     $@
      cmd_cat    = cat $^ > $@

# generate man page as plain text file
quiet_cmd_groff = GROFF   $@
      cmd_groff = groff -man -Tascii $< | col -bx > $@

# cmd macro (taken from kbuild)
cmd = @$(if $($(quiet)cmd_$(1)),echo '  $(call $(quiet)cmd_$(1),$(2))' &&) $(call cmd_$(1),$(2))

ifneq ($(dependencies),)
-include $(dependencies)
endif
