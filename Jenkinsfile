pipeline {
    agent any
    stages {
        stage('Build') {
            steps {
                echo 'Building project'
                sh 'mkdir build && cd build'
                sh 'cmake -DLLVM_SRC=~/llvm-project ..'
                sh 'cmake --build .'
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
