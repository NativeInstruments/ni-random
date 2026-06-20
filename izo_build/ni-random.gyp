{
    'targets': [
        {
            'target_name': 'ni-random',
            'type': 'none',
            'dependencies': [
            ],
            'includes': [
                'ni-random_sources.gypi',
                '../../iZFrameworks/izbuildconfig/iZBaseCommon.gypi',
                '../../iZFrameworks/izbuildconfig/FilterSources.gypi',
                '../../iZFrameworks/izbuildconfig/EnableWarnAsError.gypi',
            ],
            'all_dependent_settings': {
                'include_dirs': [
                    '../include',
                ],
            },
        },
        {
            'target_name': 'ni-randomUnitTests',
            'dependencies': [
                'ni-random',
                '../../iZFrameworks/izcore/Common/izo_build/Common.gyp:iZCoreCommon',
            ],
            'includes': [
                'ni-random_sources.gypi',
                '../../iZFrameworks/izbuildconfig/iZBaseCommon.gypi',
                '../../iZFrameworks/izbuildconfig/FilterUnitTestSources.gypi',
                '../../iZFrameworks/izbuildconfig/UnitTestConfig.gypi',
                '../../iZFrameworks/izbuildconfig/DisallowUnitTestAsserts.gypi',
            ],
        },
        {
            'target_name': 'ni-randomBenchmarks',
            'dependencies': [
                'ni-random',
                '../../iZFrameworks/izcore/Common/izo_build/Common.gyp:iZCoreCommon',
            ],
            'includes': [
                'ni-random_sources.gypi',
                '../../iZFrameworks/izbuildconfig/iZBaseCommon.gypi',
                '../../iZFrameworks/izbuildconfig/BenchmarkConfig.gypi',
                '../../iZFrameworks/izbuildconfig/FilterBenchmarkSources.gypi',
            ],
        },
    ]
}
