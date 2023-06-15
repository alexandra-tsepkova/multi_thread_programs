import os
import json
import faker
import random
import requests

from flask import Flask, request, redirect

app = Flask(__name__)
app.secret_key = "super secret key"

fake = faker.Faker()


def generate_key():
    return fake.numerify("####-####")


def create_new_database():
    ID = os.getenv("PORT")
    replica_status = {}
    pending_requests = {}
    REPLICAS = os.getenv("REPLICAS")
    if REPLICAS is not None:
        REPLICAS = REPLICAS.split(',')
        for replica in REPLICAS:
            replica_status[replica] = "ON"
            pending_requests[replica] = []
    with open(os.getcwd() + f"{ID}.json", 'w+') as f:
        f.write('{"database": {}, "replica_status": ' + json.dumps(replica_status)
                + f', "pending_requests":' + json.dumps(pending_requests) + '}')


def send_data_to_replicas(request_body, method, json_data):
    failed_replicas = []
    failed_request = {"method": method, "json": request_body}
    REPLICAS = os.getenv("REPLICAS")
    json_data = try_to_send_data_to_failed_replicas(json_data)
    if REPLICAS is not None:
        REPLICAS = REPLICAS.split(',')
        for replica in REPLICAS:
            try:
                r = requests.request(method, replica, json=request_body,
                                     headers={'Content-Type': 'application/json'})
                if r.status_code > 500:
                    failed_replicas.append(replica)
            except Exception:
                failed_replicas.append(replica)
    return failed_replicas, failed_request, json_data


def check_replica_health(replica):
    try:
        r = requests.request('OPTIONS', replica)
        return r.status_code
    except Exception:
        return 600


def update_replica_status(json_data, replica, status):
    json_data['replica_status'][replica] = status
    return json_data


def try_to_send_data_to_failed_replicas(json_data):
    pending = json_data['pending_requests']
    for replica, req_list in pending.items():
        status_code = check_replica_health(replica)
        if status_code > 500:
            update_replica_status(json_data, replica, "OFF")
        else:
            update_replica_status(json_data, replica, "ON")
        if len(req_list) != 0:
            if status_code > 500:
                break
            while len(req_list) != 0:
                req = req_list[0]
                try:
                    r = requests.request(req['method'], replica, json=req['json'],
                                         headers={'Content-Type': 'application/json'})
                    if r.status_code > 500:
                        json_data['replica_status'][replica] = "OFF"
                        break
                    else:
                        json_data['replica_status'][replica] = "ON"
                        json_data['pending_requests'][replica].remove(req)
                except Exception:
                    json_data['replica_status'][replica] = "OFF"
                    break
    return json_data


with app.app_context():
    create_new_database()


@app.get("/<key>")
def get_function(key):
    ID = os.getenv("PORT")
    REPLICAS = os.getenv("REPLICAS")
    filename = os.getcwd() + f"{ID}.json"
    with open(filename, 'r') as f:
        json_data = json.loads(f.read())
    if REPLICAS is None:
        if key in json_data['database'].keys():
            return json_data['database'][key] + '\n', 200
        else:
            return f'No key {key} in database\n', 404
    else:
        REPLICAS = REPLICAS.split(',')
        json_data = try_to_send_data_to_failed_replicas(json_data)
        with open(filename, 'w') as f:
            f.write(json.dumps(json_data, indent=3))
        for address, status in json_data['replica_status'].items():
            if status == "OFF":
                REPLICAS.remove(address)
        if len(REPLICAS) == 0:
            return 'No replicas available\n', 500
        replica = random.choice(REPLICAS)
        return redirect(replica + f'/{key}', 302)


@app.post("/")
def post_function():
    ID = os.getenv("PORT")
    request_body = request.get_json()
    filename = os.getcwd() + f"{ID}.json"
    key = str(generate_key())
    value = request_body["value"]
    request_body['key'] = key
    with open(filename, 'r') as f:
        json_data = json.loads(f.read())
        failed_replicas, failed_request, json_data = send_data_to_replicas(request_body, "PUT", json_data)
        if len(failed_replicas) > 0:
            for replica in failed_replicas:
                json_data['replica_status'][replica] = "OFF"
                json_data['pending_requests'][replica].append(failed_request)
        json_data['database'][key] = value
    with open(filename, 'w') as f:
        f.write(json.dumps(json_data, indent=3))
    return key + '\n', 201


@app.put("/")
def put_function():
    update = False
    ID = os.getenv("PORT")
    request_body = request.get_json()
    filename = os.getcwd() + f"{ID}.json"
    key = request_body["key"]
    value = request_body["value"]
    with open(filename, 'r') as f:
        json_data = json.loads(f.read())
        failed_replicas, failed_request, json_data = send_data_to_replicas(request_body, "PUT", json_data)
        if len(failed_replicas) > 0:
            for replica in failed_replicas:
                json_data['replica_status'][replica] = "OFF"
                json_data['pending_requests'][replica].append(failed_request)
        if key in json_data['database'].keys():
            update = True
        json_data['database'][key] = value
    with open(filename, 'w') as f:
        f.write(json.dumps(json_data, indent=3))
    if update:
        return '', 200
    return '', 201


@app.patch("/")
def patch_function():
    patch = False
    ID = os.getenv("PORT")
    request_body = request.get_json()
    filename = os.getcwd() + f"{ID}.json"
    key = request_body["key"]
    value = request_body["value"]
    with open(filename, 'r') as f:
        json_data = json.loads(f.read())
        failed_replicas, failed_request, json_data = send_data_to_replicas(request_body, "PATCH", json_data)
        if len(failed_replicas) > 0:
            for replica in failed_replicas:
                json_data['replica_status'][replica] = "OFF"
                json_data['pending_requests'][replica].append(failed_request)
        if key in json_data['database'].keys():
            patch = True
            json_data['database'][key] = value
    with open(filename, 'w') as f:
        f.write(json.dumps(json_data, indent=3))
    if patch:
        return '', 200
    return f'No key {key} in database\n', 404


@app.delete("/")
def delete_function():
    delete = False
    ID = os.getenv("PORT")
    request_body = request.get_json()
    filename = os.getcwd() + f"{ID}.json"
    key = request_body["key"]
    with open(filename, 'r') as f:
        json_data = json.loads(f.read())
        failed_replicas, failed_request, json_data = send_data_to_replicas(request_body, "DELETE", json_data)
        if len(failed_replicas) > 0:
            for replica in failed_replicas:
                json_data['replica_status'][replica] = "OFF"
                json_data['pending_requests'][replica].append(failed_request)
        if key in json_data['database'].keys():
            delete = True
            json_data['database'].pop(key)
    with open(filename, 'w') as f:
        f.write(json.dumps(json_data, indent=3))
    if delete:
        return '', 200
    return f'No key {key} in database\n', 404


if __name__ == '__main__':
    app.run(threaded=False)
