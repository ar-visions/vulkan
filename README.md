simple vulkan example taken from sascha williams, but restructured
into something more modular.  the idea is to ultimately use a meta
api for using attribs and uniforms and express pipeline in a more
succinct manner.

vulkan is a difficult api, and its large.  it does not make sense
to overwhelm the user with flatenning everything as an 'example'
use-case.  an example use-case should structure it in such a way
that does not obfuscate what the roles of all of the hundreds
of types are for.

the roadmap for this is to offer something of a vulkan engine that
you may write your own engine on, or, use directly.

