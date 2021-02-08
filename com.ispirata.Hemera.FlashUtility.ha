import com.ispirata.Hemera.Settings 1.0

SimpleCppApplication {
    applicationId: "com.ispirata.Hemera.FlashUtility"
    name: "FlashUtility"
    description: "Flash Utility to flash or recover Hemera devices"
    version: "0.94.9"
    organization: "Ispirata"
    features: Features.Audio | Features.Video | Features.Network | Features.Hyperspace | Features.DevicePowerManagement | Features.DeviceManagement

    resourceFiles: [
        "images/error.png",
        "images/ok.png",
        "images/warning.png"
    ]
    sourceFiles: [
        "src/fallbackscreen.cpp",
        "src/fallbackwindow.cpp",
        "src/flashtool.cpp",

        "src/imagechecksumoperation.cpp"
    ]
    rootOperations: [
        RootOperation {
            operationId: "com.ispirata.Hemera.FlashUtility.CopyRecoveryOperation"
            sourceFiles: [
                "src/copyrecoveryoperation.cpp"
            ]
        },
        RootOperation {
            operationId: "com.ispirata.Hemera.FlashUtility.MkfsOperation"
            sourceFiles: [
                "src/mkfsoperation.cpp"
            ]
        },
        RootOperation {
            operationId: "com.ispirata.Hemera.FlashUtility.DDOperation"
            sourceFiles: [
                "src/ddoperation.cpp"
            ]
        },
        RootOperation {
            operationId: "com.ispirata.Hemera.FlashUtility.EraseDirectoryOperation"
            sourceFiles: [
                "src/erasedirectoryoperation.cpp"
            ]
        },
        RootOperation {
            operationId: "com.ispirata.Hemera.FlashUtility.FlashEraseOperation"
            sourceFiles: [
                "src/flasheraseoperation.cpp"
            ]
        },
        RootOperation {
            operationId: "com.ispirata.Hemera.FlashUtility.FlashKobsOperation"
            sourceFiles: [
                "src/flashkobsoperation.cpp"
            ]
        },
        RootOperation {
            operationId: "com.ispirata.Hemera.FlashUtility.NANDWriteOperation"
            sourceFiles: [
                "src/nandwriteoperation.cpp"
            ]
        },
        RootOperation {
            operationId: "com.ispirata.Hemera.FlashUtility.PartitionTableOperation"
            sourceFiles: [
                "src/partitiontableoperation.cpp"
            ]
        },
        RootOperation {
            operationId: "com.ispirata.Hemera.FlashUtility.ToolOperation"
            sourceFiles: [
                "src/tooloperation.cpp"
            ]
        },
        RootOperation {
            operationId: "com.ispirata.Hemera.FlashUtility.UBootEnvUpdateOperation"
            sourceFiles: [
                "src/ubootenvupdateoperation.cpp"
            ]
        },
        RootOperation {
            operationId: "com.ispirata.Hemera.FlashUtility.UBootEnvBackupOperation"
            sourceFiles: [
                "src/ubootenvbackupoperation.cpp"
            ]
        },
        RootOperation {
            operationId: "com.ispirata.Hemera.FlashUtility.UBIAttachDetachOperation"
            sourceFiles: [
                "src/ubiattachdetachoperation.cpp"
            ]
        },
        RootOperation {
            operationId: "com.ispirata.Hemera.FlashUtility.UBIFormatOperation"
            sourceFiles: [
                "src/ubiformatoperation.cpp"
            ]
        },
        RootOperation {
            operationId: "com.ispirata.Hemera.FlashUtility.UBIUpdateVolOperation"
            sourceFiles: [
                "src/ubiupdatevoloperation.cpp"
            ]
        }
    ]
    qtModules: QtModules.Core | QtModules.DBus | QtModules.Gui | QtModules.Quick | QtModules.Network | QtModules.Qml
    hemeraModules: HemeraModules.Core | HemeraModules.Gui
}
