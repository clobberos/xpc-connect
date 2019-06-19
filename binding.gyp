{
  'targets': [
    {
      'target_name': 'xpc-connect',
      'conditions': [
        ['OS=="mac"', {
          'sources': [
            'src/XpcConnect.cpp'
          ],
          # cflags on OS X are stupid and have to be defined like this
          'defines': [
            '_FILE_OFFSET_BITS=64',
            '_LARGEFILE_SOURCE'
          ],
          'xcode_settings': {
            'OTHER_CFLAGS': [
              '-Wall',
              '-ObjC++'
            ]
          }
        }]
      ],
      "include_dirs" : [
            "<!(node -e \"require('nan')\")"
        ]
    }
  ]
}