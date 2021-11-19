pipeline {
    agent any

    environment {
        MAIN_BRANCH_NAME = 'main'
        IMAGE_TAG_BASE = 'registry.hub.docker.com/riftember/spf-ie'
        IMAGE_TAG_LATEST_NAME = 'latest'
    }

    stages {

        stage('Print build info') {
            steps {
                echo "Building on node ${NODE_NAME}, executor ${EXECUTOR_NUMBER}"
                echo "Build tag: ${BUILD_TAG}"
                echo "Build URL: ${BUILD_URL}"
                echo "Workspace: ${WORKSPACE}"
            }
        }

        stage('Build Docker image') {
            steps {
                sh 'podman build -t ${IMAGE_TAG_BASE}:${BUILD_TAG} .'
            }
        }

        stage('Push build-numbered image') {
            steps {
                sh 'podman push ${IMAGE_TAG_BASE}:${BUILD_TAG}'
            }
        }

        stage('Push image as latest if on main branch') {
            when {
                branch "${MAIN_BRANCH_NAME}"
            }
            steps {
                sh 'podman tag ${IMAGE_TAG_BASE}:${BUILD_TAG} ${IMAGE_TAG_BASE}:${IMAGE_TAG_LATEST_NAME}'
                sh 'podman push ${IMAGE_TAG_BASE}:${IMAGE_TAG_LATEST_NAME}'
            }
            post {
                always {
                    sh 'podman rmi ${IMAGE_TAG_BASE}:${IMAGE_TAG_LATEST_NAME}'
                }
            }
        }

        stage('Test image') {
            steps {
                sh 'podman run --rm ${IMAGE_TAG_BASE}:${BUILD_TAG} cmake --build build --target test'
            }
        }

    }

    post {
        always {
            sh 'podman rmi ${IMAGE_TAG_BASE}:${BUILD_TAG}'
        }
    }
}
