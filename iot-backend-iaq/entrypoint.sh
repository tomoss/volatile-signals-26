#!/usr/bin/env bash

set -e

main()
{
    case $1 in
        shell)
            bash
            ;;
        start)
            ./manage.py runserver 0.0.0.0:8000
            ;;
        mqtt)
            ./manage.py mqtt_client
            ;;
        migrate)
            ./manage.py migrate
            ;;
        makemigrations)
            ./manage.py makemigrations
            ;;
        *)
            printf "\t ..: Invoking '$*'\n"
            exec "$@"
            ;;
    esac
}

cd /app
main "$@"