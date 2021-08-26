pipeline {
    agent any
    stages {
        stage('Clean') {
            steps {
                echo 'Clearing previous build folder'
                dir('build') {
                    deleteDir()
                }
            }
        }
        stage('Build') {
            steps {
                echo 'Building project'
                dir('build') {
                    sh 'cmake -DLLVM_SRC=/home/jenkins-runner/llvm-project ..'
                    sh 'make'
                }
            }
        }
        stage('Test') {
            steps {
                echo 'Building and running tests'
                sh 'cmake --build build --target test'
            }
        }
    }
}
