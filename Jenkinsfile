pipeline {
    agent any
    stages {
        stage('Build') {
            steps {
                echo 'Building project'
                cmakeBuild
                    generator: 'Unix Makefiles',
                    buildDir: 'build',
                    sourceDir: '..',
                    steps: [
                        [args: '-DLLVM_SRC=/path/to/llvm-project/root']
                    ]
            }
        }
        stage('Test') {
            steps {
                echo 'Running tests'
                sh 'cmake --build build --target test'
            }
        }
    }
}
